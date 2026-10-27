/* Administrator-side prompts and complete requests. Role authorization remains on the server. */
#include "client.h"
#include <stdio.h>

/**
 * @brief Run the administrator menu, collect operation inputs, and send complete requests to
 *        the server.
 *
 * @param[in] client (Client *) Client connection and login information, including its connected
 *                   TCP socket; the socket remains owned by the caller.
 *
 * @return (void) No value. Returns after logout, keyboard input failure, or a communication
 *         failure.
 *
 * @note The caller must have authenticated as this role. Selecting zero sends LOGOUT. This
 *       function collects input locally; the server performs the requested operations.
 */
void admin_menu(Client *client)
{
    while (1)
    {
        puts("\nADMIN MENU\n1 Add user/account\n2 Modify user name\n3 Search user\n4 Delete user\n"
             "5 Close account\n6 Account details\n7 Change password\n0 Logout");
        int choice = get_choice("Choice: ", 7);
        if (choice < 0)
            return;
        char request[REQUEST_SIZE], username[64], password[128], name[128], id[32];
        switch (choice)
        {
        case 0:
            perform_request(client, "LOGOUT");
            return;
        case 1:
        {
            int role = get_choice("Role: 1 Customer, 2 Employee, 3 Manager, 0 Cancel: ", 3);
            if (role < 0)
                return;
            if (!role)
                continue;
            if (!get_text("Username: ", username, sizeof(username)) ||
                !get_text("Password: ", password, sizeof(password)) ||
                !get_text("Full name: ", name, sizeof(name)))
                return;
            snprintf(request, sizeof(request), "ADD_USER|%d|%s|%s|%s", role, username, password,
                     name);
            break;
        }
        case 2:
            if (!get_text("User ID: ", id, sizeof(id)) ||
                !get_text("New full name: ", name, sizeof(name)))
                return;
            snprintf(request, sizeof(request), "MOD_USER|%s|%s", id, name);
            break;
        case 3:
            if (!get_text("User ID: ", id, sizeof(id)))
                return;
            snprintf(request, sizeof(request), "SEARCH_USER|%s", id);
            break;
        case 4:
            if (!get_text("User ID to disable: ", id, sizeof(id)))
                return;
            snprintf(request, sizeof(request), "DELETE_USER|%s", id);
            break;
        case 5:
            if (!get_text("Account ID: ", id, sizeof(id)))
                return;
            snprintf(request, sizeof(request), "CLOSE_ACCOUNT|%s", id);
            break;
        case 6:
            if (!get_text("Account ID: ", id, sizeof(id)))
                return;
            snprintf(request, sizeof(request), "VIEW_ACCOUNT|%s", id);
            break;
        case 7:
            if (!get_text("New password: ", password, sizeof(password)))
                return;
            snprintf(request, sizeof(request), "CHANGE_PASS|%s", password);
            break;
        default:
            continue;
        }
        if (!perform_request(client, request))
            return;
    }
}
