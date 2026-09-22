#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>

// Function to detach the process and turn it into a Daemon
void daemonize() {
    pid_t pid;

    // 1. Fork off the parent process
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    
    // If we got a good PID, then we can exit the parent process.
    // This immediately returns control to the terminal.
    if (pid > 0) exit(EXIT_SUCCESS);

    // 2. Create a new session and become the session leader.
    // This fully detaches the process from the terminal that launched it.
    if (setsid() < 0) exit(EXIT_FAILURE);

    // 3. Ignore the SIGCHLD signal so child processes don't become Zombies
    signal(SIGCHLD, SIG_IGN);

    // 4. Fork a second time. (Highly recommended)
    // This ensures the daemon can NEVER re-acquire a terminal window.
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); // First child exits

    // 5. Set new file permissions (umask)
    umask(0);

    // 6. Change the working directory to the root directory.
    // This prevents the daemon from locking a directory (like a USB drive) 
    // and preventing it from being unmounted.
    chdir("/");

    // 7. Close all open file descriptors (stdin, stdout, stderr)
    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }

    // 8. Reopen standard file descriptors to /dev/null (a black hole).
    // If the daemon accidentally uses printf(), it won't crash.
    int fd0 = open("/dev/null", O_RDWR); // fd 0 (stdin)
    int fd1 = dup(0);                    // fd 1 (stdout)
    int fd2 = dup(0);                    // fd 2 (stderr)
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: ./prog <Hour(0-23)> <Minute(0-59)> <Absolute_Script_Path>\n");
        printf("Example: ./prog 14 30 /home/user/backup.sh\n");
        return 1;
    }

    int target_hour = atoi(argv[1]);
    int target_min = atoi(argv[2]);
    char *script_path = argv[3];

    // Check if path is absolute
    if (script_path[0] != '/') {
        printf("Error: You MUST provide an absolute path because daemons run from '/'.\n");
        return 1;
    }

    printf("Starting daemon. It will run in the background and execute the script at %02d:%02d.\n", target_hour, target_min);

    // Turn this process into a daemon
    daemonize();

    // Open the system log so we can write debug messages
    openlog("prog", LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "Daemon started successfully. Waiting for %02d:%02d.", target_hour, target_min);

    // --- DAEMON MAIN LOOP ---
    while (1) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);

        // Check if the current time matches our target time
        if (tm_info->tm_hour == target_hour && tm_info->tm_min == target_min) {
            syslog(LOG_NOTICE, "Time matched! Executing script: %s", script_path);

            // Create a child process to run the script
            pid_t child = fork();
            if (child == 0) {
                // We use execl to launch a shell, passing our script as an argument
                execl("/bin/sh", "sh", script_path, NULL);
                
                // If execl fails, log the error
                syslog(LOG_ERR, "Failed to execute script.");
                exit(1); 
            }

            // Exit the daemon after successful execution so it doesn't 
            // trigger again during the same minute.
            syslog(LOG_NOTICE, "Script executed. Daemon shutting down.");
            closelog();
            exit(EXIT_SUCCESS);
        }

        // Sleep for 30 seconds to prevent wasting CPU cycles in an infinite loop
        sleep(30); 
    }

    return 0;
}