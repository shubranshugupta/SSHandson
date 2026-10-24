#include <stdio.h>
#include <pthread.h>

void *task(void *arg) {
    printf("Thread ID: %lu\n", (unsigned long)pthread_self());
    return NULL;
}

int main(void) {
    pthread_t threads[3];

    for (int i = 0; i < 3; i++) {
        if (pthread_create(&threads[i], NULL, task, NULL) != 0) {
            fprintf(stderr, "Failed to create thread\n");

            for (int j = 0; j < i; j++)
                pthread_join(threads[j], NULL);

            return 1;
        }
    }

    for (int i = 0; i < 3; i++)
        pthread_join(threads[i], NULL);

    return 0;
}