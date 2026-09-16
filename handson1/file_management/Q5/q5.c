#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int pid = getpid();
    printf("Program running in background. PID is: %d\n", pid);
    
    int fds[5];
    char filename[20];
    
    for (int i = 0; i < 5; i++) {
        sprintf(filename, "file_%d.txt", i+1);
        
        fds[i] = open(filename, O_CREAT | O_WRONLY, 0644);
        
        if (fds[i] != -1) {
            printf("Created %s mapped to FD: %d\n", filename, fds[i]);
        }
    }

    printf("\n--> Now run: ls -l /proc/%d/fd\n", pid);
    printf("--> To kill this process later, run: kill %d\n", pid);
    
    while(1) {
        sleep(5); 
    }
    
    return 0;
}