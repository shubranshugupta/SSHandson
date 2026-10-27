/* Customer-side input and request construction, following the reference menu workflow. */
#include "client.h"
#include <stdio.h>

/**
 * @brief Run the customer menu, collect operation inputs, and send complete requests to the
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
void customer_menu(Client *client)
{
    while (1)
    {
        puts(
            "\nCUSTOMER MENU\n1 Balance and details\n2 Deposit\n3 Withdraw\n4 Transfer\n5 Apply for loan\n"
            "6 Change password\n7 Transaction history\n8 Feedback\n9 My loans\n10 Repay loan\n0 Logout");
        int choice = get_choice("Choice: ", 10);
        if (choice < 0)
            return;
        char request[REQUEST_SIZE], amount[40], id[32], text[256];
        switch (choice)
        {
        case 0:
            perform_request(client, "LOGOUT");
            return;
        case 1:
            snprintf(request, sizeof(request), "VIEW_BALANCE");
            break;
        case 2:
            if (!get_text("Deposit amount in Rs (e.g. 1.2): ", amount, sizeof(amount)))
                return;
            snprintf(request, sizeof(request), "DEPOSIT|%s", amount);
            break;
        case 3:
            if (!get_text("Withdrawal amount in Rs: ", amount, sizeof(amount)))
                return;
            snprintf(request, sizeof(request), "WITHDRAW|%s", amount);
            break;
        case 4:
            if (!get_text("Destination account ID: ", id, sizeof(id)) ||
                !get_text("Transfer amount in Rs: ", amount, sizeof(amount)))
                return;
            snprintf(request, sizeof(request), "TRANSFER|%s|%s", id, amount);
            break;
        case 5:
            if (!get_text("Loan amount in Rs: ", amount, sizeof(amount)))
                return;
            snprintf(request, sizeof(request), "APPLY_LOAN|%s", amount);
            break;
        case 6:
            if (!get_text("New password (8-64 characters): ", text, sizeof(text)))
                return;
            snprintf(request, sizeof(request), "CHANGE_PASS|%s", text);
            break;
        case 7:
            snprintf(request, sizeof(request), "VIEW_HISTORY");
            break;
        case 8:
            if (!get_text("Feedback: ", text, sizeof(text)))
                return;
            snprintf(request, sizeof(request), "ADD_FEEDBACK|%s", text);
            break;
        case 9:
            snprintf(request, sizeof(request), "VIEW_LOANS");
            break;
        case 10:
            if (!get_text("Loan ID: ", id, sizeof(id)) ||
                !get_text("Repayment in Rs: ", amount, sizeof(amount)))
                return;
            snprintf(request, sizeof(request), "REPAY_LOAN|%s|%s", id, amount);
            break;
        default:
            continue;
        }
        /* All fields have been collected before the single request is sent. */
        if (!perform_request(client, request))
            return;
    }
}
