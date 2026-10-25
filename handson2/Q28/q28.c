#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int main(int argc, char *argv[]) {
    struct msqid_ds info;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <message_queue_id>\n", argv[0]);
        return 1;
    }

    int msgid = atoi(argv[1]);

    if (msgctl(msgid, IPC_STAT, &info) == -1) {
        perror("IPC_STAT");
        return 1;
    }

    printf("Old permissions: %03o\n",
           (unsigned int)(info.msg_perm.mode & 0777));

    info.msg_perm.mode = 0666; /* Read/write for everyone. */

    if (msgctl(msgid, IPC_SET, &info) == -1) {
        perror("IPC_SET");
        return 1;
    }

    printf("New permissions: 666\n");
    return 0;
}