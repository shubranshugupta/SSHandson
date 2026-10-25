#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>

void print_time(const char *label, time_t value) {
    if (value == 0)
        printf("%s: Not set\n", label);
    else
        printf("%s: %s", label, ctime(&value));
}

int main(int argc, char *argv[]) {
    struct msqid_ds info;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <message_queue_id>\n", argv[0]);
        return 1;
    }

    int msgid = atoi(argv[1]);

    if (msgctl(msgid, IPC_STAT, &info) == -1) {
        perror("msgctl");
        return 1;
    }

    struct ipc_perm *perm = &info.msg_perm;

    printf("Access permissions: %03o\n",
           (unsigned int)(perm->mode & 0777));

    printf("Owner UID: %lu\n", (unsigned long)perm->uid);
    printf("Owner GID: %lu\n", (unsigned long)perm->gid);

    print_time("Last message sent", info.msg_stime);
    print_time("Last message received", info.msg_rtime);
    print_time("Last queue change", info.msg_ctime);

    printf("Current message data size: %lu bytes\n",
           (unsigned long)info.msg_cbytes);

    printf("Number of messages: %lu\n",
           (unsigned long)info.msg_qnum);

    printf("Maximum bytes allowed: %lu\n",
           (unsigned long)info.msg_qbytes);

    printf("Last msgsnd PID: %ld\n", (long)info.msg_lspid);
    printf("Last msgrcv PID: %ld\n", (long)info.msg_lrpid);

    return 0;
}