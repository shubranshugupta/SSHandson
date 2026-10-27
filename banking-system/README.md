# Banking system: client menus and role-specific server requests

The client asks which role to log in as, collects username/password locally, and runs the selected client module. Every operation's fields are collected on the client before a complete request is sent. The server never sends an INPUT prompt.

This follows the menu/action-request architecture of the supplied GitHub project, adapted to your requirements: C, MySQL, one pthread per client, plain-text application passwords, and Indian rupee amounts with two decimal places.

## Source layout

```text
src/client.c                  login role menu and connection
src/client/customer.c         customer menu and complete requests
src/client/admin.c            administrator menu and complete requests
src/client/manager.c          manager menu and complete requests
src/client/employee.c         employee menu and complete requests
src/client/network.c          keyboard input and send_request()

src/server.c                  client threads, login and authenticated dispatch
src/server/customer.c         deposits, withdrawals, transfers, loans, feedback
src/server/admin.c            user/account administration
src/server/manager.c          status changes, loan assignment, feedback review
src/server/employee.c         customer maintenance and loan processing
src/server/auth.c             login and password changes
src/server/users.c            shared user/account functions
src/server/database.c         MySQL and transaction helpers
src/server/init_admin.c       initial administrator creation
src/common.c                  shared TCP framing and validation

include/client.h              client declarations
include/server.h              server declarations and session
include/common.h              role IDs, limits and shared declarations
schema.sql                    MySQL schema with one customer per account
```

The role modules are functions linked into bank_client.out, not separate executable processes. Selecting a role calls customer_menu(), admin_menu(), manager_menu(), or employee_menu(). These functions call send_request() through perform_request().

## Login and operation flow

```text
client.c
  -> select Customer / Administrator / Manager / Employee / Exit
  -> enter username and password locally
  -> send LOGIN|role_id|username|password
  -> server authenticates and checks the role
  -> run the corresponding client menu
  -> collect every field locally
  -> send a complete operation request
  -> server runs that role's handler and replies
  -> display result and repeat
```

Example:

```text
Customer selects Deposit and enters 1.2
Client sends:  DEPOSIT|1.2
Server:       validates Rs 1.20, locks the account, updates MySQL and history
Server sends: SUCCESS|DEPOSIT completed. Rs 1.20
```

The old READY/INPUT prompt protocol is removed. Each request is one newline-ended line. Each response starts with SUCCESS| or ERROR| and ends with a separate END line. END marks the end of a response. LOGOUT also closes the login connection, then the client returns to its login menu. Exit terminates the client.

## Setup

1. Clone the repository:

   ```bash
   git clone https://github.com/shubranshugupta/SSHandson.git
   ```

2. Install the required packages and start MySQL:

   ```bash
   sudo apt update
   sudo apt install build-essential mysql-server libmysqlclient-dev
   sudo service mysql start
   ```

3. Open the project folder and build:

   ```bash
   cd banking-system/
   make
   ```

   If you saved the project elsewhere, use that folder instead.

4. From the same project folder, open the MySQL command line:

   ```bash
   sudo mysql
   ```

   At the MySQL prompt, load schema.sql and create the application's database user:

   ```sql
   SOURCE schema.sql;
   CREATE USER IF NOT EXISTS 'bankapp'@'localhost' IDENTIFIED BY 'BankDbPass123!';
   GRANT SELECT, INSERT, UPDATE, DELETE ON simple_bank.* TO 'bankapp'@'localhost';
   EXIT;
   ```

   The password above is an example. Use the same database password in MySQL and config.env. If bankapp already exists, use its existing password.

5. Create `config.env` with your database settings and initial administrator password, then load it in the terminal:
   ```ini
   export BANK_DB_HOST=127.0.0.1
   export BANK_DB_PORT=3306
   export BANK_DB_USER=bankapp
   export BANK_DB_PASSWORD='BankApp@123'
   export BANK_DB_NAME=simple_bank
   export BANK_ADMIN_PASSWORD='Admin@123'
   export SERVER_IP=127.0.0.1
   export SERVER_PORT=8080
   ```

   ```bash
   source ./config.env
   ```

   `BANK_DB_PASSWORD` must match the bankapp database user's password. `BANK_ADMIN_PASSWORD` is the initial banking administrator's password and must contain 8-64 allowed characters. The C programs read the exported environment variables; they do not load config.env automatically.

6. Create the initial banking administrator:

   ```bash
   ./bank_init.out
   ```

   Log in to the banking application with username admin and the password set in `BANK_ADMIN_PASSWORD`. If an administrator already exists, setup keeps that account and its password unchanged. Additional administrators cannot be created through
   the application.

## Run

Server terminal, with the database environment set:

```bash
./bank_server.out 127.0.0.1 8080
```

Client terminal:

```bash
./bank_client.out 127.0.0.1 8080
```

Client login menu:

```text
1 Customer
2 Administrator
3 Manager
4 Employee
0 Exit
```

For another machine, bind the server to 0.0.0.0 and give the client the server's reachable IPv4 address. The server listens on TCP port 8080 in this example. Only the server needs database credentials. Build BOTH new binaries before using this replacement protocol. No build or tests were run by the assistant.

## Roles

- Customer: balance/details, deposit, withdraw, transfer, loan application, password change, history, feedback, loan status and repayment.
- Employee: add customers, modify customer names, process assigned loans, inspect customer passbooks and assigned loans, change password.
- Manager: activate/deactivate accounts, assign loans to employees, review feedback and loans, change password.
- Administrator: create customers, employees, and managers; modify/search/disable users; close zero-balance accounts; view account details; change password.

Role IDs in requests: 1=Customer, 2=Employee, 3=Manager, 4=Administrator. 

The server checks login credentials and the selected role, then uses its authenticated session for all later operations. Client-supplied user IDs cannot impersonate the logged-in user.

Each customer, employee, manager, and administrator may have only one active login. After validating credentials, the server atomically obtains GET_LOCK with timeout zero for a name derived from the database and user ID. A second login receives an already-logged-in error, including simultaneous login attempts on different server processes connected to the same MySQL instance and database. The database name is shortened with MD5 only to fit MySQL's lock-name length limit; passwords remain plain text as requested.

Login ownership is held by the client's private MySQL connection and checked before authenticated operations. Transaction COMMIT and ROLLBACK do not release it. Logout closes that connection before sending SUCCESS. Disconnect, failed login after lock acquisition, and worker cleanup also close it. Idle clients remain logged in with no application-imposed socket timeout. An abrupt network failure releases ownership when the operating system or MySQL detects the disconnection. Database connection termination releases its named locks, so there is no stored logged-in flag to reset after a restart. All server instances must run the updated code for this policy. No file locks, schema changes, or migrations are required.

Log in as admin to create customers, employees, and managers. Creating a customer automatically creates a separate account with a balance of Rs 0.00. Each customer has one account; account IDs are unique across user records. Staff have no account. The server prints new user and account IDs after the transaction commits.

Loans: customer applies -> manager assigns -> assigned employee approves/rejects. Approval and account credit are one transaction. Repayment reduces principal; interest calculations are outside this mini project. Deletion disables a user's login and retains financial history. Closure deactivates a zero-balance account without unresolved loans; a manager can later reactivate it.

## Money, synchronization and ACID

Enter amounts in rupees: 1.2 means Rs 1.20 and 0.2 means Rs 0.20. Positive values with at most two decimal places are allowed. There is no conversion to integer paisa and no floating-point arithmetic. MySQL DECIMAL performs the money arithmetic.

- Atomicity: balance changes and transaction history commit together; failures roll back. Loan approval/disbursement and repayment also use transactions.
- Consistency: positive-amount validation, balance checks/limits, active account checks, foreign keys, CHECK constraints and controlled loan state transitions.
- Isolation: each client thread has its own MySQL connection. FOR UPDATE row locks serialize competing changes. Transfers lock account IDs in increasing order.
- Durability: success is sent after COMMIT. Use InnoDB with `innodb_flush_log_at_trx_commit=1`, and `sync_binlog=1` if binary logging is enabled, on storage that honors flush requests.

Use MySQL 8.0.16+ so CHECK constraints are enforced. List responses show at most 100 records; the underlying records remain in MySQL. The server supports up to 64 client threads with no application-imposed socket timeout. A query failure or deadlock cancels its operation without automatic retry. If a COMMIT result is uncertain, inspect history before retrying.

Application passwords are plain text, as requested. TCP messages are also plain text. Text inputs use letters, digits, spaces and _-.@!#$%+?; quotes, backslashes, newlines and the | separator are not accepted in SQL text fields. This is a basic classroom implementation for a trusted lab network.

## Reference and backup

Reference: https://github.com/dharani070707/Banking_Management_System

This is an original implementation of that client-menu/named-action pattern, adapted for MySQL and pthreads. It is not a byte-for-byte copy of its SQLite/fork implementation. See docs/ARCHITECTURE.md for the diagrams and request list.

