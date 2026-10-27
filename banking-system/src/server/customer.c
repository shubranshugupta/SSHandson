/* Complete customer requests arrive here after the client collects every field. */
#include "server.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Perform a deposit, withdrawal, or transfer with locked balances and an atomic history
 *        entry.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] action (const char *) Trusted action string: DEPOSIT, WITHDRAW, or TRANSFER.
 * @param[in] target_text (const char *) Destination account-ID text for TRANSFER; ignored for
 *                        the other two actions.
 * @param[in] amount_text (const char *) Positive rupee amount text, accepting at most two
 *                        fractional digits.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (void) No value. Writes SUCCESS after commit, or ERROR if validation, locking,
 *         balance rules, or SQL fails.
 *
 * @pre Call for an authenticated customer with no active application transaction.
 *
 * @note Refreshes and locks the active user, locks required account rows in ascending ID order,
 *       updates balances, records history, and commits together.
 *
 * @note A transfer cannot target the same account. Failures before commit roll back the
 *       operation.
 */
static void transact(Session *s, const char *action, const char *target_text,
                     const char *amount_text, char *reply)
{
    int deposit = !strcmp(action, "DEPOSIT");
    int transfer = !strcmp(action, "TRANSFER");
    int target = transfer ? parse_id(target_text) : 0;
    char amount[MONEY_SIZE];
    if (!rupee_amount(amount_text, amount) || (transfer && target <= 0))
    {
        snprintf(reply, RESPONSE_SIZE,
                 "ERROR|Enter positive rupees with up to two decimal places.");
        return;
    }
    if (!begin_transaction(s, reply))
        return;
    if (!current_user(s, 1, reply))
        goto failed;
    int source = s->account_id;
    char balance[MONEY_SIZE], other_balance[MONEY_SIZE];
    int active = 0, other_active = 0;
    if (transfer && target == source)
        goto failed;
    /* Lock accounts in increasing ID order to reduce transfer deadlocks. */
    if (transfer && target < source)
    {
        if (!lock_account(s, target, other_balance, &other_active, reply) ||
            !lock_account(s, source, balance, &active, reply))
            goto failed;
    }
    else
    {
        if (!lock_account(s, source, balance, &active, reply))
            goto failed;
        if (transfer && !lock_account(s, target, other_balance, &other_active, reply))
            goto failed;
    }
    if (!active || (transfer && !other_active))
        goto failed;
    if (!update_balance(s, source, amount, deposit, reply))
        goto failed;
    if (transfer && !update_balance(s, target, amount, 1, reply))
        goto failed;
    /* Balance changes and their history row are committed together. */
    if (!record_transaction(s, action, deposit ? 0 : source, deposit ? source : target, amount,
                            reply))
        goto failed;
    if (commit_transaction(s, reply))
        snprintf(reply, RESPONSE_SIZE, "SUCCESS|%s completed. Rs %s", action, amount);
    return;
failed:
    cancel_transaction(s);
    snprintf(
        reply, RESPONSE_SIZE,
        "ERROR|Transaction cancelled. Check funds, account status, destination, and balance limit.");
}

/**
 * @brief Create a pending loan request for the logged-in customer's active account.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] value (const char *) Requested positive loan amount in rupees, with at most two
 *                  fractional digits.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (void) No value. Writes the committed loan ID on success, or an ERROR reply.
 *
 * @note Locks the user and account and commits the loan request in one transaction. Applying
 *       does not credit the balance; approval performs disbursement.
 */
static void apply_loan(Session *s, const char *value, char *reply)
{
    char amount[MONEY_SIZE], balance[MONEY_SIZE], sql[512];
    int active;
    if (!rupee_amount(value, amount))
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid rupee amount.");
        return;
    }
    if (!begin_transaction(s, reply))
        return;
    if (!current_user(s, 1, reply) || !lock_account(s, s->account_id, balance, &active, reply) ||
        !active)
        goto failed;
    snprintf(sql, sizeof(sql),
             "INSERT INTO loans(account_id,applicant_id,amount,remaining) VALUES(%d,%d,%s,%s)",
             s->account_id, s->user_id, amount, amount);
    if (!execute_sql(s, sql, reply))
        goto failed;
    int id = (int)mysql_insert_id(s->db);
    if (commit_transaction(s, reply))
        snprintf(reply, RESPONSE_SIZE, "SUCCESS|Loan ID=%d requested. Rs %s", id, amount);
    return;
failed:
    cancel_transaction(s);
    snprintf(reply, RESPONSE_SIZE, "ERROR|Loan application cancelled.");
}

/**
 * @brief Debit a customer account and reduce an approved loan balance in one transaction.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] id_text (const char *) Positive loan-ID text; the loan must belong to the
 *                    logged-in account.
 * @param[in] value (const char *) Positive repayment amount in rupees, no greater than the
 *                  remaining loan amount.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (void) No value. Writes SUCCESS after the debit, loan update, and history record
 *         commit, or an ERROR reply.
 *
 * @note Locks the approved loan and account, requires an active account with sufficient funds,
 *       and marks a fully repaid loan as repaid. Any failure before commit rolls back the
 *       operation.
 */
static void repay_loan(Session *s, const char *id_text, const char *value, char *reply)
{
    int id = parse_id(id_text);
    char amount[MONEY_SIZE], balance[MONEY_SIZE], sql[512];
    if (id <= 0 || !rupee_amount(value, amount))
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid loan or amount.");
        return;
    }
    if (!begin_transaction(s, reply))
        return;
    if (!current_user(s, 1, reply))
        goto failed;
    /* Loan first, account second: the same order used by employee approval. */
    snprintf(sql, sizeof(sql),
             "SELECT id FROM loans WHERE id=%d AND account_id=%d AND status='approved' "
             "AND remaining>=%s FOR UPDATE",
             id, s->account_id, amount);
    MYSQL_RES *result = select_sql(s, sql, reply);
    if (!result)
        goto failed;
    int found = mysql_num_rows(result) == 1;
    mysql_free_result(result);
    int active;
    if (!found || !lock_account(s, s->account_id, balance, &active, reply) || !active)
        goto failed;
    if (!update_balance(s, s->account_id, amount, 0, reply))
        goto failed;
    /* Determine status from the old remaining amount before subtracting payment. */
    snprintf(sql, sizeof(sql),
             "UPDATE loans SET status=CASE WHEN remaining=%s THEN 'repaid' ELSE 'approved' END,"
             "remaining=remaining-%s WHERE id=%d",
             amount, amount, id);
    if (!execute_sql(s, sql, reply) ||
        !record_transaction(s, "REPAYMENT", s->account_id, 0, amount, reply))
        goto failed;
    if (commit_transaction(s, reply))
        snprintf(reply, RESPONSE_SIZE, "SUCCESS|Loan repayment completed. Rs %s", amount);
    return;
failed:
    cancel_transaction(s);
    snprintf(reply, RESPONSE_SIZE,
             "ERROR|Repayment cancelled. Check loan, remaining amount, and funds.");
}

/**
 * @brief Dispatch customer requests for balances, money operations, loans, history, and
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
 * @note The outer dispatcher must authenticate the CUSTOMER role. Recognized requests write
 *       their result into reply.
 */
int handle_customer_action(Session *s, char **v, int n, char *reply)
{
    if ((!strcmp(v[0], "VIEW_BALANCE") || !strcmp(v[0], "VIEW_DETAILS")) && n == 1)
        view_account(s, s->account_id, reply);
    else if (!strcmp(v[0], "DEPOSIT") && n == 2)
        transact(s, v[0], "0", v[1], reply);
    else if (!strcmp(v[0], "WITHDRAW") && n == 2)
        transact(s, v[0], "0", v[1], reply);
    else if (!strcmp(v[0], "TRANSFER") && n == 3)
        transact(s, v[0], v[1], v[2], reply);
    else if (!strcmp(v[0], "APPLY_LOAN") && n == 2)
        apply_loan(s, v[1], reply);
    else if (!strcmp(v[0], "REPAY_LOAN") && n == 3)
        repay_loan(s, v[1], v[2], reply);
    else if (!strcmp(v[0], "VIEW_HISTORY") && n == 1)
        view_history(s, s->account_id, reply);
    else if (!strcmp(v[0], "VIEW_LOANS") && n == 1)
    {
        char sql[512];
        snprintf(sql, sizeof(sql),
                 "SELECT id,amount AS amount_Rs,remaining AS remaining_Rs,status,employee_id "
                 "FROM loans WHERE account_id=%d ORDER BY id DESC LIMIT 100",
                 s->account_id);
        query_table(s, sql, reply);
    }
    else if (!strcmp(v[0], "ADD_FEEDBACK") && n == 2)
    {
        if (!valid_text(v[1], 1, 200))
        {
            snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid feedback text.");
            return 1;
        }
        char sql[512];
        snprintf(sql, sizeof(sql), "INSERT INTO feedback(user_id,message) VALUES(%d,'%s')",
                 s->user_id, v[1]);
        if (execute_sql(s, sql, reply))
            snprintf(reply, RESPONSE_SIZE, "SUCCESS|Feedback saved.");
    }
    else
        return 0;
    return 1;
}
