/* Each worker opens its own MySQL connection. InnoDB coordinates concurrent writes. */
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Read an environment variable or use a supplied fallback string.
 *
 * @param[in] name (const char *) Null-terminated environment variable name.
 * @param[in] fallback (const char *) Value used when that environment variable is absent.
 *
 * @return (const char *) A borrowed pointer to the environment value, or fallback when the
 *         variable is absent.
 *
 * @note Does not load config.env automatically. Do not free the returned pointer.
 */
static const char *setting(const char *name, const char *fallback)
{
    const char *value = getenv(name);
    return value ? value : fallback;
}

/**
 * @brief Open a MySQL connection using BANK_DB_* environment settings and READ COMMITTED
 *        isolation.
 *
 * @par Parameters
 * None (void).
 *
 * @return (MYSQL *) An owned MYSQL connection pointer, or NULL if initialization, connection,
 *         or isolation setup fails.
 *
 * @note Defaults are host 127.0.0.1, port 3306, user bankapp, empty password, and database
 *       simple_bank.
 *
 * @note The caller must call mysql_close() on success. Connections belong to one client thread
 *       and are not shared between threads.
 */
MYSQL *open_database(void)
{
    MYSQL *db = mysql_init(NULL);
    if (!db)
        return NULL;
    unsigned int timeout = 5;
    mysql_options(db, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    if (!mysql_real_connect(db, setting("BANK_DB_HOST", "127.0.0.1"),
                            setting("BANK_DB_USER", "bankapp"), setting("BANK_DB_PASSWORD", ""),
                            setting("BANK_DB_NAME", "simple_bank"),
                            (unsigned int)atoi(setting("BANK_DB_PORT", "3306")), NULL, 0))
    {
        fprintf(stderr, "MySQL connection: %s\n", mysql_error(db));
        mysql_close(db);
        return NULL;
    }
    if (mysql_query(db, "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED"))
    {
        mysql_close(db);
        return NULL;
    }
    return db;
}

/**
 * @brief Execute one server-generated SQL statement on the session connection.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] sql (const char *) Null-terminated SQL statement assembled by trusted server code
 *                from validated values.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (int) 1 if mysql_query() succeeds; 0 on SQL failure, with an ERROR message written to
 *         reply.
 *
 * @note Does not check affected-row counts or commit an existing explicit transaction.
 *       Successful calls need not change reply.
 */
int execute_sql(Session *s, const char *sql, char *reply)
{
    if (mysql_query(s->db, sql))
    {
        fprintf(stderr, "MySQL: %s\n", mysql_error(s->db));
        snprintf(reply, RESPONSE_SIZE, "ERROR|Database operation failed.");
        return 0;
    }
    return 1;
}

/**
 * @brief Execute a SELECT statement and buffer its result rows in memory.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] sql (const char *) Null-terminated server-generated SELECT statement.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (MYSQL_RES *) An owned MYSQL_RES pointer on success; NULL on query or
 *         result-buffering failure, with an ERROR reply.
 *
 * @note The caller must release the result with mysql_free_result(). Freeing a result does not
 *       release row locks held by a transaction.
 */
MYSQL_RES *select_sql(Session *s, const char *sql, char *reply)
{
    if (!execute_sql(s, sql, reply))
        return NULL;
    MYSQL_RES *result = mysql_store_result(s->db);
    if (!result)
        snprintf(reply, RESPONSE_SIZE, "ERROR|Could not read database result.");
    return result;
}

/**
 * @brief Start an explicit transaction on this session's MySQL connection.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (int) 1 when START TRANSACTION succeeds; 0 on failure with an ERROR reply.
 *
 * @pre No other application transaction should already be active on this connection.
 *
 * @note The caller must finish the transaction with commit_transaction() or
 *       cancel_transaction().
 */
int begin_transaction(Session *s, char *reply)
{
    return execute_sql(s, "START TRANSACTION", reply);
}

/**
 * @brief Ask MySQL to roll back the session's uncommitted transaction.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 *
 * @return (void) No value. The mysql_rollback() result is not checked by this helper.
 *
 * @note Used on operation failure and during connection cleanup. It cannot undo a transaction
 *       that has already committed.
 */
void cancel_transaction(Session *s)
{
    mysql_rollback(s->db);
}

/**
 * @brief Commit the transaction and report an uncertain commit outcome if confirmation fails.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (int) 1 when COMMIT is confirmed; 0 if confirmation fails, after a rollback attempt
 *         and an ERROR reply.
 *
 * @note An unconfirmed commit may already have reached the database. The reply instructs the
 *       user to inspect history before retrying.
 */
int commit_transaction(Session *s, char *reply)
{
    if (execute_sql(s, "COMMIT", reply))
        return 1;
    cancel_transaction(s);
    snprintf(reply, RESPONSE_SIZE, "ERROR|Commit not confirmed. Check history before retrying.");
    return 0;
}

/**
 * @brief Verify that the logged-in user remains active and refresh the session role and
 *        account.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] lock (int) Nonzero requests LOCK IN SHARE MODE on the user row; zero uses an
 *                 ordinary SELECT.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (int) 1 if an active user is found and the session is refreshed; 0 on
 *         missing/inactive user or SQL failure.
 *
 * @pre When lock is nonzero, call inside an explicit transaction to retain the row lock.
 *
 * @note The locking form prevents another transaction from changing the selected user row until
 *       this transaction ends.
 */
int current_user(Session *s, int lock, char *reply)
{
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT role,account_id FROM users WHERE id=%d AND active=1%s",
             s->user_id, lock ? " LOCK IN SHARE MODE" : "");
    MYSQL_RES *result = select_sql(s, sql, reply);
    if (!result)
        return 0;
    MYSQL_ROW row = mysql_fetch_row(result);
    int found = row != NULL;
    if (found)
    {
        s->role = role_number(row[0]);
        s->account_id = row[1] ? atoi(row[1]) : 0;
    }
    mysql_free_result(result);
    if (!found)
        snprintf(reply, RESPONSE_SIZE, "ERROR|User is inactive or missing.");
    return found;
}

/**
 * @brief Lock one account row for update and read its current balance and active flag.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] id (int) Positive account identifier.
 * @param[out] balance (char *) Writable buffer of at least MONEY_SIZE bytes for the decimal
 *                     balance string.
 * @param[out] active (int *) Pointer receiving 1 for an active account or 0 for an inactive
 *                    account.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (int) 1 if the account exists and outputs are populated; 0 if the account is absent
 *         or the query fails.
 *
 * @pre Call inside an explicit transaction; the FOR UPDATE lock is held until commit or
 *      rollback.
 *
 * @note An absent account does not necessarily write an ERROR reply; the caller supplies the
 *       appropriate operation error. Outputs are valid only on success.
 */
int lock_account(Session *s, int id, char *balance, int *active, char *reply)
{
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT balance,active FROM accounts WHERE id=%d FOR UPDATE", id);
    MYSQL_RES *result = select_sql(s, sql, reply);
    if (!result)
        return 0;
    MYSQL_ROW row = mysql_fetch_row(result);
    int found = row != NULL;
    if (found)
    {
        snprintf(balance, MONEY_SIZE, "%s", row[0]);
        *active = atoi(row[1]);
    }
    mysql_free_result(result);
    return found;
}

/**
 * @brief Credit or debit an active account using MySQL DECIMAL arithmetic and balance limits.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] id (int) Positive identifier of the account to update.
 * @param[in] amount (const char *) Positive rupee amount as null-terminated decimal text.
 * @param[in] credit (int) Nonzero to add the amount; zero to subtract it.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (int) 1 if exactly one account row is updated; 0 for invalid amount, missing/inactive
 *         account, failed balance constraints, or SQL failure.
 *
 * @pre The caller must own the account row lock and include this update in the operation's
 *      explicit transaction.
 *
 * @note A credit cannot exceed MAX_RUPEES and a debit cannot make the balance negative.
 *       Validation or rule failures may leave reply unchanged.
 */
int update_balance(Session *s, int id, const char *amount, int credit, char *reply)
{
    char sql[512], rupees[MONEY_SIZE];
    if (!rupee_amount(amount, rupees))
        return 0;
    if (credit)
        snprintf(sql, sizeof(sql),
                 "UPDATE accounts SET balance=balance+%s WHERE id=%d "
                 "AND active=1 AND balance<=%s-%s",
                 rupees, id, MAX_RUPEES, rupees);
    else
        snprintf(sql, sizeof(sql),
                 "UPDATE accounts SET balance=balance-%s WHERE id=%d "
                 "AND active=1 AND balance>=%s",
                 rupees, id, rupees);
    return execute_sql(s, sql, reply) && mysql_affected_rows(s->db) == 1;
}

/**
 * @brief Insert a financial history entry using the logged-in user as its actor.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] kind (const char *) Trusted server-defined transaction kind, such as DEPOSIT or
 *                 TRANSFER.
 * @param[in] from (int) Source account identifier; zero is stored as SQL NULL.
 * @param[in] to (int) Destination account identifier; zero is stored as SQL NULL.
 * @param[in] amount (const char *) Positive rupee amount as null-terminated decimal text.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (int) 1 when the history row is inserted; 0 for an invalid amount or SQL failure.
 *
 * @pre Use the same explicit transaction as the corresponding balance changes so the history
 *      and money update commit together.
 *
 * @note This helper does not commit. Amount-validation failure can leave reply unchanged.
 */
int record_transaction(Session *s, const char *kind, int from, int to, const char *amount,
                       char *reply)
{
    char sql[512], source[32], target[32], rupees[MONEY_SIZE];
    if (!rupee_amount(amount, rupees))
        return 0;
    snprintf(source, sizeof(source), "%d", from);
    snprintf(target, sizeof(target), "%d", to);
    snprintf(sql, sizeof(sql),
             "INSERT INTO transactions(actor_id,from_account,to_account,kind,amount) "
             "VALUES(%d,%s,%s,'%s',%s)",
             s->user_id, from ? source : "NULL", to ? target : "NULL", kind, rupees);
    return execute_sql(s, sql, reply);
}

/**
 * @brief Format a SELECT result as a SUCCESS response containing column names and text rows.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] sql (const char *) Trusted server-generated SELECT statement, normally limited to
 *                100 rows.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (void) No value. Writes a table or an ERROR message into reply.
 *
 * @note Prints SQL NULL as a dash and stops adding rows before the response buffer fills.
 *       Releases its MYSQL_RES internally.
 */
void query_table(Session *s, const char *sql, char *reply)
{
    MYSQL_RES *result = select_sql(s, sql, reply);
    if (!result)
        return;
    size_t used = (size_t)snprintf(reply, RESPONSE_SIZE, "SUCCESS|All amounts are in Rs.\n");
    unsigned int count = mysql_num_fields(result);
    MYSQL_FIELD *fields = mysql_fetch_fields(result);
    for (unsigned int i = 0; i < count; i++)
        used += (size_t)snprintf(reply + used, RESPONSE_SIZE - used, "%s%s", i ? " | " : "",
                                 fields[i].name);
    used += (size_t)snprintf(reply + used, RESPONSE_SIZE - used, "\n");
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
        for (unsigned int i = 0; i < count; i++)
        {
            const char *value = row[i] ? row[i] : "-";
            if (used + strlen(value) + 6 >= RESPONSE_SIZE)
                goto full;
            used +=
                (size_t)snprintf(reply + used, RESPONSE_SIZE - used, "%s%s", i ? " | " : "", value);
        }
        used += (size_t)snprintf(reply + used, RESPONSE_SIZE - used, "\n");
    }
full:
    mysql_free_result(result);
}
