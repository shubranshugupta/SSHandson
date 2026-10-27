/* Manager-side prompts and request construction. */
#include "client.h"
#include <stdio.h>

/**
 * @brief Run the manager menu, collect operation inputs, and send complete requests to the
 *        server.
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
void manager_menu(Client *client)
{
    while (1)
    {
        puts(
            "\nMANAGER MENU\n1 Activate or deactivate account\n2 Assign loan\n3 Review feedback\n4 All loans\n5 Change password\n0 Logout");
        int choice = get_choice("Choice: ", 5);
        if (choice < 0)
            return;
        char request[REQUEST_SIZE], id[32], employee[32], password[128];
        switch (choice)
        {
        case 0:
            perform_request(client, "LOGOUT");
            return;
        case 1:
        {
            if (!get_text("Account ID: ", id, sizeof(id)))
                return;
            int active = get_choice("1 Active, 0 Inactive: ", 1);
            if (active < 0)
                return;
            snprintf(request, sizeof(request), "STATUS_ACCOUNT|%s|%d", id, active);
            break;
        }
        case 2:
            if (!get_text("Loan ID: ", id, sizeof(id)) ||
                !get_text("Employee user ID: ", employee, sizeof(employee)))
                return;
            snprintf(request, sizeof(request), "ASSIGN_LOAN|%s|%s", id, employee);
            break;
        case 3:
            snprintf(request, sizeof(request), "VIEW_FEEDBACK");
            break;
        case 4:
            snprintf(request, sizeof(request), "VIEW_LOANS");
            break;
        case 5:
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
