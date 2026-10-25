#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int main(void) {
    /* Use an existing file to generate the key. */
    key_t key = ftok(".", 'A');
    if (key == (key_t)-1) {
        perror("ftok");
        return 1;
    }

    int msgid = msgget(key, IPC_CREAT | 0600);
    if (msgid == -1) {
        perror("msgget");
        return 1;
    }

    printf("Message queue key: 0x%lx\n", (unsigned long)key);
    printf("Message queue ID: %d\n", msgid);

    return 0;
}