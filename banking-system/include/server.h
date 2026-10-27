#ifndef SERVER_H
#define SERVER_H
#include "common.h"
#include <mysql.h>
typedef struct
{
    MYSQL *db;   /* Private to one client thread. */
    int user_id; /* Set by login, never trusted from operation requests. */
    int role;
    int account_id;
} Session;

/**
 * @brief Verify that this session database connection still owns its exclusive per-user login
 *        lock.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (int) 1 if IS_USED_LOCK matches CONNECTION_ID; 0 for missing identity, lost
 *         ownership, or database failure.
 *
 * @note The dispatcher closes the client session when ownership cannot be verified.
 */
int check_login(Session *s, char *reply);

/**
 * @brief Close the private MySQL connection to release its login lock, then clear the session
 *        identity.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 *
 * @return (void) No value. Sets the database pointer to NULL and the user, role, and account
 *         IDs to zero.
 *
 * @note Safe to call again on an already closed session. MySQL connection termination also
 *       releases named locks after an abnormal disconnect.
 */
void close_session(Session *s);

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
MYSQL *open_database(void);

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
int execute_sql(Session *s, const char *sql, char *reply);

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
MYSQL_RES *select_sql(Session *s, const char *sql, char *reply);

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
int begin_transaction(Session *s, char *reply);

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
void cancel_transaction(Session *s);

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
int commit_transaction(Session *s, char *reply);

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
int current_user(Session *s, int lock, char *reply);

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
int lock_account(Session *s, int id, char *balance, int *active, char *reply);

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
int update_balance(Session *s, int id, const char *amount, int credit, char *reply);

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
                       char *reply);

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
void query_table(Session *s, const char *sql, char *reply);

/**
 * @brief Validate credentials, reserve one active login per user, revalidate current
 *        credentials, and populate the session.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] v (char **) Request field array: LOGIN, role ID, username, and password.
 * @param[in] n (int) Request field count; a login requires exactly 4.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (void) No value. Writes SUCCESS with the user, role, and account IDs, or an ERROR
 *         reply.
 *
 * @note Requires an active user and an exact role/password match. Duplicate logins are rejected
 *       atomically for all roles. Credentials are rechecked after acquiring the login lock.
 *
 * @note The lock survives transaction commits/rollbacks and is released when close_session()
 *       closes the connection. Failed post-lock validation closes the session.
 */
void handle_login(Session *s, char **v, int n, char *reply);

/**
 * @brief Replace the logged-in active user's stored plain-text password.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] password (const char *) Null-terminated new password with 8 through 64 allowed
 *                     characters.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (void) No value. Writes SUCCESS when the UPDATE query succeeds, or ERROR on invalid
 *         input or SQL failure.
 *
 * @note Uses one UPDATE statement. The caller must authenticate the session before calling.
 */
void change_password(Session *s, const char *password, char *reply);

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
                 char *reply);

/**
 * @brief Update a user's display name, optionally restricting the update to customers.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] id (const char *) Null-terminated positive user-ID text.
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
void rename_user(Session *s, const char *id, const char *name, int customer_only, char *reply);

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
void view_account(Session *s, int id, char *reply);

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
void view_history(Session *s, int id, char *reply);

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
int handle_customer_action(Session *s, char **v, int n, char *reply);

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
int handle_employee_action(Session *s, char **v, int n, char *reply);

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
int handle_manager_action(Session *s, char **v, int n, char *reply);

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
int handle_admin_action(Session *s, char **v, int n, char *reply);
#endif
