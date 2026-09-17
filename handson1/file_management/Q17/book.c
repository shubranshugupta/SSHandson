#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    // 1. Open the file in Read/Write mode
    int fd = open("ticket.txt", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }

    // 2. Initialize the lock structure
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;    // Request a Write (Exclusive) Lock
    lock.l_whence = SEEK_SET; // Start locking from the beginning of the file
    lock.l_start = 0;         // Offset is 0
    lock.l_len = 0;           // 0 means lock the entire file

    printf("Waiting for booking server... (Trying to acquire lock)\n");
    
    // 3. Apply the lock
    // F_SETLKW means "Wait". If another person is currently booking, this 
    // program will freeze right here until the other person finishes.
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Error acquiring lock");
        close(fd);
        return 1;
    }
    
    printf("Lock acquired! You are securely inside the booking system.\n");

    // 4. Read the current ticket number
    char buffer[16] = {0};
    read(fd, buffer, sizeof(buffer) - 1);
    
    // Convert string to integer
    int ticket_num = atoi(buffer);
    printf("Current ticket available: %d\n", ticket_num);

    // 5. Increment the ticket
    ticket_num++;
    printf("Booking confirmed. Next ticket will be: %d\n", ticket_num);

    // Simulate network delay / database processing time (5 seconds)
    printf("[Simulating heavy processing... Please wait]\n");
    sleep(10);

    // 6. Write the new ticket number back to the file
    // We must move the file pointer back to the beginning before writing!
    lseek(fd, 0, SEEK_SET);
    
    char write_buf[16];
    int len = sprintf(write_buf, "%d\n", ticket_num);
    write(fd, write_buf, len);

    // 7. Release the lock
    lock.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        perror("Error releasing lock");
    } else {
        printf("Lock released. Transaction complete.\n");
    }

    close(fd);
    return 0;
}