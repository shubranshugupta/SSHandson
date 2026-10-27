/* User/account management shared by the administrator and employee handlers. */
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a customer, employee, or manager; customers receive a new account in the same
 *        transaction.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] role (int) New user role: CUSTOMER, EMPLOYEE, or MANAGER. The ADMIN role is
 *                 rejected.
 * @param[in] username (const char *) Null-terminated username containing 1 through 32 allowed
 *                     characters.
 * @param[in] password (const char *) Null-terminated plain-text password containing 8 through
 *                     64 allowed characters.
 * @param[in] name (const char *) Null-terminated display name containing 1 through 64 allowed
 *                 characters.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (void) No value. Writes committed user/account IDs on success, or an ERROR reply on
 *         failure.
 *
 * @note Each customer receives a separate new account with a zero balance. Staff receive a NULL
 *       account ID. The user insert and any account insert commit or roll back together.
 *
 * @note Administrator creation is always rejected here. The caller enforces which other roles
 *       it may create; an employee passes CUSTOMER.
 */
void create_user(Session *s, int role, const char *username, const char *password, const char *name,
                 char *reply)
{
    int account = 0;
    if (role == ADMIN)
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Creating another administrator is not allowed.");
        return;
    }
    if (role < CUSTOMER || role > MANAGER || !valid_text(username, 1, 32) ||
        !valid_text(password, 8, 64) || !valid_text(name, 1, 64))
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid role, profile, or password.");
        return;
    }
    if (!begin_transaction(s, reply))
        return;
    if (!current_user(s, 1, reply))
        goto failed;
    char sql[1024];
    if (role == CUSTOMER)
    {
        /* Every customer receives a separate account in the same transaction. */
        if (!execute_sql(s, "INSERT INTO accounts(balance) VALUES(0)", reply))
            goto failed;
        account = (int)mysql_insert_id(s->db);
    }
    char account_sql[32];
    snprintf(account_sql, sizeof(account_sql), "%d", account);
    snprintf(sql, sizeof(sql),
             "INSERT INTO users(username,name,password,role,account_id) "
             "VALUES('%s','%s','%s','%s',%s)",
             username, name, password, role_name(role), account ? account_sql : "NULL");
    if (!execute_sql(s, sql, reply))
        goto failed;
    int id = (int)mysql_insert_id(s->db);
    if (commit_transaction(s, reply))
        snprintf(reply, RESPONSE_SIZE, "SUCCESS|User ID=%d Account ID=%d", id, account);
    return;
failed:
    cancel_transaction(s);
    snprintf(reply, RESPONSE_SIZE,
             "ERROR|Creation cancelled. Check username and database constraints.");
}

/**
 * @brief Update a user's display name, optionally restricting the update to customers.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] id_text (const char *) Null-terminated positive user-ID text.
 * @param[in] name (const char *) New display name containing 1 through 64 allowed characters.
 * @param[in] customer_only (int) Nonzero restricts the target role to customer; zero allows any
 *                          user role.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (void) No value. Writes the number of changed rows on SQL success, or an ERROR reply
 *         for invalid input or query failure.
 *
 * @note An unchanged name or unmatched user can produce zero changed rows. The caller enforces
 *       authorization.
 */
void rename_user(Session *s, const char *id_text, const char *name, int customer_only, char *reply)
{
    int id = parse_id(id_text);
    if (id <= 0 || !valid_text(name, 1, 64))
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid user or name.");
        return;
    }
    char sql[512];
    snprintf(sql, sizeof(sql), "UPDATE users SET name='%s' WHERE id=%d%s", name, id,
             customer_only ? " AND role='customer'" : "");
    if (execute_sql(s, sql, reply))
        snprintf(reply, RESPONSE_SIZE, "SUCCESS|Changed rows: %llu. An unchanged name gives 0.",
                 (unsigned long long)mysql_affected_rows(s->db));
}

/**
 * @brief Read an account and its customer details and format the result as a response table.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] id (int) Positive account identifier whose details should be displayed.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (void) No value. Writes a table or an ERROR reply.
 *
 * @note The caller decides which account the session may view. A nonexistent account produces
 *       an empty result table.
 */
void view_account(Session *s, int id, char *reply)
{
    if (id <= 0)
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid account.");
        return;
    }
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT a.id,a.balance AS balance_Rs,a.active,u.id AS customer_id,u.name "
             "FROM accounts a JOIN users u ON u.account_id=a.id WHERE a.id=%d LIMIT 100",
             id);
    query_table(s, sql, reply);
}

/**
 * @brief Read the latest 100 history entries involving an account as source or destination.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] id (int) Positive account identifier whose transaction history should be
 *               displayed.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (void) No value. Writes the history table or an ERROR reply.
 *
 * @note The caller authorizes access to the selected account. Results are ordered from newest
 *       to oldest.
 */
void view_history(Session *s, int id, char *reply)
{
    if (id <= 0)
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid account.");
        return;
    }
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id,kind,amount AS amount_Rs,from_account,to_account,created_at "
             "FROM transactions WHERE from_account=%d OR to_account=%d ORDER BY id DESC LIMIT 100",
             id, id);
    query_table(s, sql, reply);
}
