/* Manager requests assign loans, change account status and review feedback. */
#include "server.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Assign a pending loan to an active employee inside a locked transaction.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] loan (int) Positive identifier of the pending loan.
 * @param[in] employee (int) Positive user identifier of an active employee.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (void) No value. Writes SUCCESS after assignment commits, or an ERROR reply.
 *
 * @note Locks the loan for update and holds a shared lock on the selected employee row while
 *       checking its role and active status.
 */
static void assign_loan(Session *s, int loan, int employee, char *reply)
{
    if (loan <= 0 || employee <= 0)
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid IDs.");
        return;
    }
    if (!begin_transaction(s, reply))
        return;
    if (!current_user(s, 1, reply))
        goto failed;
    char sql[512];
    snprintf(sql, sizeof(sql), "SELECT id FROM loans WHERE id=%d AND status='pending' FOR UPDATE",
             loan);
    MYSQL_RES *result = select_sql(s, sql, reply);
    if (!result)
        goto failed;
    int found = mysql_num_rows(result) == 1;
    mysql_free_result(result);
    if (!found)
        goto failed;
    snprintf(sql, sizeof(sql),
             "SELECT id FROM users WHERE id=%d AND role='employee' AND active=1 LOCK IN SHARE MODE",
             employee);
    result = select_sql(s, sql, reply);
    if (!result)
        goto failed;
    found = mysql_num_rows(result) == 1;
    mysql_free_result(result);
    if (!found)
        goto failed;
    snprintf(sql, sizeof(sql), "UPDATE loans SET employee_id=%d WHERE id=%d", employee, loan);
    if (!execute_sql(s, sql, reply))
        goto failed;
    if (commit_transaction(s, reply))
        snprintf(reply, RESPONSE_SIZE, "SUCCESS|Loan assigned.");
    return;
failed:
    cancel_transaction(s);
    snprintf(reply, RESPONSE_SIZE,
             "ERROR|Assignment cancelled. Pending loan and active employee required.");
}

/**
 * @brief Dispatch manager requests for account status, loan assignment, loan lists, and
 *        feedback.
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
 * @note The outer dispatcher must authenticate the MANAGER role. Account status changes use one
 *       UPDATE statement.
 */
int handle_manager_action(Session *s, char **v, int n, char *reply)
{
    if (!strcmp(v[0], "STATUS_ACCOUNT") && n == 3)
    {
        int account = parse_id(v[1]), active = parse_id(v[2]);
        if (account <= 0 || active < 0 || active > 1)
        {
            snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid account/status.");
            return 1;
        }
        char sql[256];
        snprintf(sql, sizeof(sql), "UPDATE accounts SET active=%d WHERE id=%d", active, account);
        if (execute_sql(s, sql, reply))
            snprintf(reply, RESPONSE_SIZE, "SUCCESS|Changed rows: %llu.",
                     (unsigned long long)mysql_affected_rows(s->db));
    }
    else if (!strcmp(v[0], "ASSIGN_LOAN") && n == 3)
        assign_loan(s, parse_id(v[1]), parse_id(v[2]), reply);
    else if (!strcmp(v[0], "VIEW_LOANS") && n == 1)
        query_table(
            s,
            "SELECT id,account_id,amount AS amount_Rs,remaining AS remaining_Rs,status,employee_id "
            "FROM loans ORDER BY id DESC LIMIT 100",
            reply);
    else if (!strcmp(v[0], "VIEW_FEEDBACK") && n == 1)
        query_table(s, "SELECT id,user_id,message FROM feedback ORDER BY id DESC LIMIT 100", reply);
    else
        return 0;
    return 1;
}
