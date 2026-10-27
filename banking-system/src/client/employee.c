/* Employee-side prompts and complete requests. This file does not execute SQL. */
#include "client.h"
#include <stdio.h>

/**
 * @brief Run the employee menu, collect operation inputs, and send complete requests to the
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
void employee_menu(Client *client)
{
    while (1)
    {
        puts("\nEMPLOYEE MENU\n1 Add customer\n2 Modify customer name\n3 Approve or reject loan\n"
             "4 Customer passbook\n5 Assigned loans\n6 Change password\n0 Logout");
        int choice = get_choice("Choice: ", 6);
        if (choice < 0)
            return;
        char request[REQUEST_SIZE], username[64], password[128], name[128], id[32];
        switch (choice)
        {
        case 0:
            perform_request(client, "LOGOUT");
            return;
        case 1:
            if (!get_text("Username: ", username, sizeof(username)) ||
                !get_text("Password: ", password, sizeof(password)) ||
                !get_text("Full name: ", name, sizeof(name)))
                return;
            snprintf(request, sizeof(request), "ADD_CUSTOMER|%s|%s|%s", username, password, name);
            break;
        case 2:
            if (!get_text("Customer user ID: ", id, sizeof(id)) ||
                !get_text("New full name: ", name, sizeof(name)))
                return;
            snprintf(request, sizeof(request), "MOD_CUSTOMER|%s|%s", id, name);
            break;
        case 3:
        {
            if (!get_text("Loan ID: ", id, sizeof(id)))
                return;
            int decision = get_choice("1 Approve, 2 Reject, 0 Cancel: ", 2);
            if (decision < 0)
                return;
            if (!decision)
                continue;
            snprintf(request, sizeof(request), "PROCESS_LOAN|%s|%s", id,
                     decision == 1 ? "APPROVE" : "REJECT");
            break;
        }
        case 4:
            if (!get_text("Account ID: ", id, sizeof(id)))
                return;
            snprintf(request, sizeof(request), "VIEW_PASSBOOK|%s", id);
            break;
        case 5:
            snprintf(request, sizeof(request), "VIEW_ASSIGNED_LOANS");
            break;
        case 6:
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
