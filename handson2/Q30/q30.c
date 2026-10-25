#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

int main(void) {
    int shmid = shmget(IPC_PRIVATE, 1024, IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    /* a. Attach with read/write access and write data. */
    char *data = shmat(shmid, NULL, 0);
    if (data == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    strcpy(data, "Hello shared memory!");
    printf("Shared memory ID: %d\n", shmid);
    printf("Written: %s\n", data);

    /* c. Detach the writable attachment. */
    if (shmdt(data) == -1) {
        perror("shmdt");
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    /* b. Attach read-only and attempt an overwrite. */
    char *readonly = shmat(shmid, NULL, SHM_RDONLY);
    if (readonly == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    printf("Read-only attachment: %s\n", readonly);
    fflush(stdout);

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
    } else if (pid == 0) {
        volatile char *p = readonly;
        p[0] = 'X'; /* Causes SIGSEGV on Linux. */
        _exit(0);
    } else {
        int status;

        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
        } else if (WIFSIGNALED(status) &&
                   WTERMSIG(status) == SIGSEGV) {
            printf("Overwrite failed: child received SIGSEGV.\n");
        }

        printf("Data remains: %s\n", readonly);
    }

    /* c. Detach the read-only attachment. */
    if (shmdt(readonly) == -1)
        perror("shmdt");
    else
        printf("Shared memory detached.\n");

    /* d. Remove the shared memory segment. */
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        return 1;
    }

    printf("Shared memory removed.\n");
    return 0;
}