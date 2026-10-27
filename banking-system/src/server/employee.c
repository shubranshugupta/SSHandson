/* Employee request handlers. Only an assigned employee can process a pending loan. */
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Approve or reject a pending loan assigned to the logged-in employee.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] id_text (const char *) Positive loan-ID text.
 * @param[in] decision (const char *) Exact decision string: APPROVE or REJECT.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (void) No value. Writes SUCCESS after the decision commits, or an ERROR reply.
 *
 * @note Locks the assigned pending loan. Approval also locks and credits its active account and
 *       inserts history in the same transaction; rejection only changes loan status.
 */
static void process_loan(Session *s, const char *id_text, const char *decision, char *reply)
{
    int id = parse_id(id_text);
    int approve = !strcmp(decision, "APPROVE");
    if (id <= 0 || (!approve && strcmp(decision, "REJECT")))
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid loan decision.");
        return;
    }
    if (!begin_transaction(s, reply))
        return;
    if (!current_user(s, 1, reply))
        goto failed;
    char sql[512], amount[MONEY_SIZE];
    snprintf(sql, sizeof(sql),
             "SELECT account_id,amount FROM loans WHERE id=%d AND employee_id=%d "
             "AND status='pending' FOR UPDATE",
             id, s->user_id);
    MYSQL_RES *result = select_sql(s, sql, reply);
    if (!result)
        goto failed;
    MYSQL_ROW row = mysql_fetch_row(result);
    int account = row ? atoi(row[0]) : 0;
    int valid = row && rupee_amount(row[1], amount);
    mysql_free_result(result);
    if (!account || !valid)
        goto failed;
    if (approve)
    {
        char balance[MONEY_SIZE];
        int active;
        if (!lock_account(s, account, balance, &active, reply) || !active)
            goto failed;
        if (!update_balance(s, account, amount, 1, reply) ||
            !record_transaction(s, "LOAN_CREDIT", 0, account, amount, reply))
            goto failed;
    }
    snprintf(sql, sizeof(sql), "UPDATE loans SET status='%s' WHERE id=%d",
             approve ? "approved" : "rejected", id);
    if (!execute_sql(s, sql, reply))
        goto failed;
    if (commit_transaction(s, reply))
        snprintf(reply, RESPONSE_SIZE, "SUCCESS|Loan %s.",
                 approve ? "approved and credited" : "rejected");
    return;
failed:
    cancel_transaction(s);
    snprintf(reply, RESPONSE_SIZE,
             "ERROR|Processing cancelled. Check assignment, status, and account.");
}

/**
 * @brief Dispatch employee requests for customer maintenance, assigned loans, and passbooks.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] v (char **) Array of n null-terminated request fields; v[0] is the action name.
 * @param[in] n (int) Number of elements in v, including the action name.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (int) 1 if the action and field count are recognized, including requests that produce
 *         an ERROR reply; 0 if the action or field count is unsupported. This is not an
 *         operation-success flag.
 *
 * @note The outer dispatcher must authenticate the EMPLOYEE role. New users created here are
 *       restricted to the CUSTOMER role.
 */
int handle_employee_action(Session *s, char **v, int n, char *reply)
{
    if (!strcmp(v[0], "ADD_CUSTOMER") && n == 4)
        create_user(s, CUSTOMER, v[1], v[2], v[3], reply);
    else if (!strcmp(v[0], "MOD_CUSTOMER") && n == 3)
        rename_user(s, v[1], v[2], 1, reply);
    else if (!strcmp(v[0], "PROCESS_LOAN") && n == 3)
        process_loan(s, v[1], v[2], reply);
    else if (!strcmp(v[0], "VIEW_PASSBOOK") && n == 2)
        view_history(s, parse_id(v[1]), reply);
    else if (!strcmp(v[0], "VIEW_ASSIGNED_LOANS") && n == 1)
    {
        char sql[512];
        snprintf(sql, sizeof(sql),
                 "SELECT id,account_id,amount AS amount_Rs,status FROM loans "
                 "WHERE employee_id=%d ORDER BY id DESC LIMIT 100",
                 s->user_id);
        query_table(s, sql, reply);
    }
    else
        return 0;
    return 1;
}
