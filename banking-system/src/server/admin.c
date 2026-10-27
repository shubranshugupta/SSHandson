/* Administrator requests maintain users and close eligible accounts. */
#include "server.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Deactivate a zero-balance account that has no pending or approved loans.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] id (int) Positive identifier of the account to close.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (void) No value. Writes SUCCESS after closure commits, or an ERROR reply.
 *
 * @note Locks the user and account and checks unresolved loans in one transaction. Closure
 *       preserves the account record and its financial history.
 */
static void close_account(Session *s, int id, char *reply)
{
    if (id <= 0)
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid account.");
        return;
    }
    if (!begin_transaction(s, reply))
        return;
    char balance[MONEY_SIZE];
    int active;
    if (!current_user(s, 1, reply) || !lock_account(s, id, balance, &active, reply) ||
        strcmp(balance, "0.00"))
        goto failed;
    char sql[256];
    snprintf(sql, sizeof(sql),
             "SELECT id FROM loans WHERE account_id=%d AND status IN ('pending','approved')", id);
    MYSQL_RES *result = select_sql(s, sql, reply);
    if (!result)
        goto failed;
    int unresolved = mysql_num_rows(result) != 0;
    mysql_free_result(result);
    if (unresolved)
        goto failed;
    snprintf(sql, sizeof(sql), "UPDATE accounts SET active=0 WHERE id=%d", id);
    if (!execute_sql(s, sql, reply))
        goto failed;
    if (commit_transaction(s, reply))
        snprintf(reply, RESPONSE_SIZE, "SUCCESS|Account closed; records retained.");
    return;
failed:
    cancel_transaction(s);
    snprintf(reply, RESPONSE_SIZE,
             "ERROR|Closure cancelled. Zero balance and no unresolved loans required.");
}

/**
 * @brief Dispatch administrator requests for user creation, renaming, search, disabling, and
 *        account closure.
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
 * @note The outer dispatcher must authenticate the ADMIN role. DELETE_USER disables a non-admin
 *       user rather than physically deleting its record.
 */
int handle_admin_action(Session *s, char **v, int n, char *reply)
{
    if (!strcmp(v[0], "ADD_USER") && n == 5)
        create_user(s, parse_id(v[1]), v[2], v[3], v[4], reply);
    else if (!strcmp(v[0], "MOD_USER") && n == 3)
        rename_user(s, v[1], v[2], 0, reply);
    else if (!strcmp(v[0], "SEARCH_USER") && n == 2)
    {
        int id = parse_id(v[1]);
        if (id <= 0)
        {
            snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid user.");
            return 1;
        }
        char sql[256];
        snprintf(sql, sizeof(sql),
                 "SELECT id,username,name,role,active,account_id FROM users WHERE id=%d", id);
        query_table(s, sql, reply);
    }
    else if (!strcmp(v[0], "DELETE_USER") && n == 2)
    {
        int id = parse_id(v[1]);
        if (id <= 0)
        {
            snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid user.");
            return 1;
        }
        char sql[256];
        snprintf(sql, sizeof(sql), "UPDATE users SET active=0 WHERE id=%d AND role<>'admin'", id);
        if (execute_sql(s, sql, reply))
            snprintf(reply, RESPONSE_SIZE, "SUCCESS|Users disabled: %llu; records retained.",
                     (unsigned long long)mysql_affected_rows(s->db));
    }
    else if (!strcmp(v[0], "CLOSE_ACCOUNT") && n == 2)
        close_account(s, parse_id(v[1]), reply);
    else if (!strcmp(v[0], "VIEW_ACCOUNT") && n == 2)
        view_account(s, parse_id(v[1]), reply);
    else
        return 0;
    return 1;
}
