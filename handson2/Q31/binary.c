#include <stdio.h>
#include <sys/ipc.h>
#include <sys/sem.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int main(void) {
    int semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (semid == -1) {
        perror("semget");
        return 1;
    }

    union semun arg;
    arg.val = 1;

    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("SETVAL");
        semctl(semid, 0, IPC_RMID);
        return 1;
    }

    printf("Binary semaphore ID: %d\n", semid);
    printf("Initial value: %d\n", arg.val);

    return 0;
}