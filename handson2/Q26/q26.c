#include <stdio.h>
#include <string.h>
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

    int msgid = msgget(key, IPC_CREAT | 0600);
    if (msgid == -1) {
        perror("msgget");
        return 1;
    }

    msg.type = 1; /* Message type must be positive. */

    printf("Enter message: ");
    fflush(stdout);

    if (fgets(msg.text, sizeof(msg.text), stdin) == NULL) {
        fprintf(stderr, "Could not read message\n");
        return 1;
    }

    /* The size excludes the leading long type field. */
    if (msgsnd(msgid, &msg, strlen(msg.text) + 1, 0) == -1) {
        perror("msgsnd");
        return 1;
    }

    printf("Message sent. Queue ID: %d\n", msgid);
    return 0;
}