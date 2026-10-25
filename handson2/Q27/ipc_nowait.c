#include <stdio.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct message {
    long type;
    char text[100];
};

int main(void) {
    struct message msg;

    key_t key = ftok(".", 'A');
    if (key == (key_t)-1) {
        perror("ftok");
        return 1;
    }

    int msgid = msgget(key, 0600);
    if (msgid == -1) {
        perror("msgget");
        return 1;
    }

    ssize_t count = msgrcv(msgid, &msg, sizeof(msg.text), 1, IPC_NOWAIT);

    if (count == -1) {
        if (errno == ENOMSG)
            printf("No type 1 message is available.\n");
        else {
            perror("msgrcv");
            return 1;
        }
    } else {
        printf("Received: %.*s\n", (int)count, msg.text);
    }

    return 0;
}