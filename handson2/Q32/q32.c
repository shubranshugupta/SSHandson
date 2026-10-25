#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

struct shared_data {
    int ticket;
    char message[100];
};

/* -1 acquires a semaphore; +1 releases it. */
void change_sem(int semid, unsigned short number, short value) {
    struct sembuf operation = {number, value, SEM_UNDO};

    while (semop(semid, &operation, 1) == -1) {
        if (errno == EINTR)
            continue;

        perror("semop");
        exit(1);
    }
}

int main(void) {
    int shmid, semid;
    struct shared_data *data;
    unsigned short initial_values[2] = {1, 2};
    union semun arg;
    int result = 0;

    setbuf(stdout, NULL);

    shmid = shmget(IPC_PRIVATE, sizeof(struct shared_data),
                   IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    data = shmat(shmid, NULL, 0);
    if (data == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    data->ticket = 0;
    data->message[0] = '\0';

    semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0600);
    if (semid == -1) {
        perror("semget");
        shmdt(data);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    arg.array = initial_values;

    if (semctl(semid, 0, SETALL, arg) == -1) {
        perror("SETALL");
        semctl(semid, 0, IPC_RMID);
        shmdt(data);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    for (int i = 0; i < 5; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            result = 1;
            break;
        }

        if (pid == 0) {
            /* a, b. Protect ticket creation and shared writes. */
            change_sem(semid, 0, -1);

            int ticket = ++data->ticket;

            snprintf(data->message, sizeof(data->message),
                     "PID %ld created ticket %d",
                     (long)getpid(), ticket);

            printf("%s\n", data->message);

            change_sem(semid, 0, 1);

            /* c. Only two processes may enter at a time. */
            change_sem(semid, 1, -1);

            printf("Ticket %d: using a pseudo resource\n", ticket);
            sleep(2);
            printf("Ticket %d: releasing the resource\n", ticket);

            change_sem(semid, 1, 1);

            shmdt(data);
            exit(0);
        }
    }

    /* Wait for all children before removing IPC objects. */
    for (;;) {
        int status;
        pid_t pid = wait(&status);

        if (pid == -1) {
            if (errno == EINTR)
                continue;
            break;
        }

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
            result = 1;
    }

    printf("Total tickets created: %d\n", data->ticket);

    /* d. Remove the semaphore set. */
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("IPC_RMID semaphore");
        result = 1;
    } else {
        printf("Semaphore set removed.\n");
    }

    shmdt(data);

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("IPC_RMID shared memory");
        result = 1;
    }

    return result;
}