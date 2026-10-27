/* Run once to create the first administrator; existing users are not overwritten. */
#include "server.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Create the initial administrator using BANK_ADMIN_PASSWORD only when no administrator
 *        exists.
 *
 * @par Parameters
 * None (void).
 *
 * @return (int) 0 if the administrator is created or already exists; 1 for an invalid password,
 *         MySQL initialization failure, or database/query failure.
 *
 * @note Accepts no command-line arguments. The password must contain 8 through 64 allowed
 *       characters and is stored as plain text.
 *
 * @note Checks for any administrator row before inserting. An existing administrator is
 *       retained without resetting its password.
 *
 * @note Uses the fixed unique username admin, so concurrent setup processes cannot create two
 *       administrators.
 */
int main(void)
{
    const char *password = getenv("BANK_ADMIN_PASSWORD");
    if (!password || !valid_text(password, 8, 64))
    {
        fprintf(stderr, "Set BANK_ADMIN_PASSWORD to 8-64 allowed characters.\n");
        return 1;
    }
    if (mysql_library_init(0, NULL, NULL))
        return 1;
    MYSQL *db = open_database();
    if (!db)
    {
        mysql_library_end();
        return 1;
    }
    int error = 1;
    /* Count any existing administrator, including an inactive one. */
    if (mysql_query(db, "SELECT id FROM users WHERE role='admin' LIMIT 1"))
    {
        fprintf(stderr, "Cannot check administrator: %s\n", mysql_error(db));
        goto done;
    }
    MYSQL_RES *result = mysql_store_result(db);
    if (!result)
    {
        fprintf(stderr, "Cannot read administrator result: %s\n", mysql_error(db));
        goto done;
    }
    int exists = mysql_num_rows(result) != 0;
    mysql_free_result(result);
    if (exists)
    {
        puts("Administrator already exists. No new administrator was created.");
        error = 0;
        goto done;
    }
    char sql[512];
    /* The fixed unique username also prevents two initializers creating two admins. */
    snprintf(
        sql, sizeof(sql),
        "INSERT INTO users(username,name,password,role) VALUES('admin','Administrator','%s','admin')",
        password);
    error = mysql_query(db, sql);
    if (error)
        fprintf(stderr, "Cannot create administrator: %s\n", mysql_error(db));
    else
        puts("Administrator created. Username: admin. Password: supplied BANK_ADMIN_PASSWORD.");
done:
    mysql_close(db);
    mysql_library_end();
    return error ? 1 : 0;
}
