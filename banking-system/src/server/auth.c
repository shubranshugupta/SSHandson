/* Authentication is performed only by the server, using plain-text application passwords. */
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Atomically reserve one active login for a validated user using a connection-owned
 *        MySQL named lock.
 *
 * @param[in,out] s (Session *) Client session containing its private MySQL connection and its
 *                  current user, role, and account identifiers.
 * @param[in] user_id (int) Positive user ID obtained from a successful credential lookup.
 * @param[out] reply (char *) Writable response buffer with capacity of at least RESPONSE_SIZE
 *                   bytes.
 *
 * @return (int) 1 when the lock is acquired and s->user_id is reserved; 0 for an existing login
 *         or a database/lock failure.
 *
 * @pre The session has an open private connection and no authenticated user. The caller must
 *      revalidate credentials or close the session.
 *
 * @note Uses GET_LOCK with timeout zero. SQL/result errors close the database connection to
 *       release any lock acquired before the error.
 *
 * @note The database identifier is shortened for namespacing; no password hashing is performed.
 */
static int claim_login(Session *s, int user_id, char *reply)
{
    char sql[256];
    /* Hash only the database name to keep the namespaced lock shorter than 64 characters. */
    snprintf(sql, sizeof(sql), "SELECT GET_LOCK(CONCAT('banking.login.',MD5(DATABASE()),'.',%d),0)",
             user_id);
    MYSQL_RES *result = select_sql(s, sql, reply);
    if (!result)
    {
        /* The query may have acquired the lock before buffering its result failed. */
        close_session(s);
        return 0;
    }
    MYSQL_ROW row = mysql_fetch_row(result);
    int acquired = row && row[0] ? atoi(row[0]) : -1;
    mysql_free_result(result);
    if (acquired != 1)
    {
        snprintf(reply, RESPONSE_SIZE,
                 acquired == 0
                     ? "ERROR|This user is already logged in. Log out from the other client first."
                     : "ERROR|Could not reserve the login session.");
        if (acquired != 0)
            close_session(s);
        return 0;
    }
    s->user_id = user_id;
    return 1;
}

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
int check_login(Session *s, char *reply)
{
    if (!s->db || s->user_id <= 0)
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Login required.");
        return 0;
    }
    char sql[256];
    snprintf(sql, sizeof(sql),
             "SELECT IS_USED_LOCK(CONCAT('banking.login.',MD5(DATABASE()),'.',%d))=CONNECTION_ID()",
             s->user_id);
    MYSQL_RES *result = select_sql(s, sql, reply);
    if (!result)
        return 0;
    MYSQL_ROW row = mysql_fetch_row(result);
    int owned = row && row[0] && !strcmp(row[0], "1");
    mysql_free_result(result);
    if (!owned)
        snprintf(reply, RESPONSE_SIZE,
                 "ERROR|Login session is no longer active. Please reconnect.");
    return owned;
}

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
void close_session(Session *s)
{
    if (s->db)
    {
        /* Closing this connection releases its named login lock, even after SQL errors. */
        mysql_close(s->db);
        s->db = NULL;
    }
    s->user_id = 0;
    s->role = 0;
    s->account_id = 0;
}

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
void handle_login(Session *s, char **v, int n, char *reply)
{
    if (!s->db || s->user_id || n != 4 || !valid_text(v[2], 1, 32))
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid login request.");
        return;
    }
    int requested = parse_id(v[1]);
    if (requested < CUSTOMER || requested > ADMIN)
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid role.");
        return;
    }
    char sql[256];
    snprintf(sql, sizeof(sql),
             "SELECT id,role,account_id,password FROM users WHERE username='%s' AND active=1",
             v[2]);
    MYSQL_RES *result = select_sql(s, sql, reply);
    if (!result)
        return;
    MYSQL_ROW row = mysql_fetch_row(result);
    int valid = row && row[3] && !strcmp(row[3], v[3]) && role_number(row[1]) == requested;
    int user_id = valid ? atoi(row[0]) : 0;
    mysql_free_result(result);
    if (user_id <= 0)
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Invalid credentials, role, or inactive user.");
        return;
    }
    if (!claim_login(s, user_id, reply))
        return;

    /* Recheck credentials and active state after acquiring exclusive login ownership. */
    result = select_sql(s, sql, reply);
    if (!result)
    {
        close_session(s);
        return;
    }
    row = mysql_fetch_row(result);
    valid = row && atoi(row[0]) == user_id && row[3] && !strcmp(row[3], v[3]) &&
            role_number(row[1]) == requested;
    int account_id = valid && row[2] ? atoi(row[2]) : 0;
    mysql_free_result(result);
    if (!valid || !check_login(s, reply))
    {
        close_session(s);
        snprintf(reply, RESPONSE_SIZE,
                 "ERROR|Login state changed. Please reconnect and log in again.");
        return;
    }
    s->role = requested;
    s->account_id = account_id;
    snprintf(reply, RESPONSE_SIZE, "SUCCESS|%d|%d|%d", s->user_id, s->role, s->account_id);
}

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
void change_password(Session *s, const char *password, char *reply)
{
    if (!valid_text(password, 8, 64))
    {
        snprintf(reply, RESPONSE_SIZE, "ERROR|Password needs 8-64 allowed characters.");
        return;
    }
    char sql[512];
    snprintf(sql, sizeof(sql), "UPDATE users SET password='%s' WHERE id=%d AND active=1", password,
             s->user_id);
    if (execute_sql(s, sql, reply))
        snprintf(reply, RESPONSE_SIZE, "SUCCESS|Password changed.");
}
