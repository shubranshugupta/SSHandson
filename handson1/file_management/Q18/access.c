#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

struct Record {
    int id;
    int data_value;
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        write(2, "Usage: ./access_record <record_number 1-5> <r|w>\n", 49);
        return 1;
    }

    int rec_num = atoi(argv[1]);
    char lock_type = argv[2][0];

    if (rec_num < 1 || rec_num > 5) {
        printf("Please choose record 1, 2, 3, 4, or 5.\n");
        return 1;
    }

    // 1. Open the file in Read/Write mode
    int fd = open("database.dat", O_RDWR);
    if (fd == -1) {
        perror("Error opening database");
        return 1;
    }

    // 2. Configure the Record Lock
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    
    // a. & b. Set lock type to Read (Shared) or Write (Exclusive)
    if (lock_type == 'r') {
        lock.l_type = F_RDLCK;
    } else if (lock_type == 'w') {
        lock.l_type = F_WRLCK;
    } else {
        printf("Invalid lock type. Use 'r' or 'w'.\n");
        return 1;
    }

    // Isolate the specific bytes for this record
    lock.l_whence = SEEK_SET;
    lock.l_start = (rec_num - 1) * sizeof(struct Record); // Calculate start offset
    lock.l_len = sizeof(struct Record);                   // Lock exactly one record size

    printf("Waiting to acquire lock on Record %d...\n", rec_num);

    // 3. Apply the lock (F_SETLKW will Wait/Block if someone else has the lock)
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Error acquiring lock");
        close(fd);
        return 1;
    }
    printf("Lock successfully acquired on Record %d.\n", rec_num);

    // 4. Move file pointer to the start of our record and read it
    lseek(fd, lock.l_start, SEEK_SET);
    
    struct Record rec;
    read(fd, &rec, sizeof(struct Record));
    printf("Current Record Data -> ID: %d, Value: %d\n", rec.id, rec.data_value);

    // 5. If it is a Write lock, modify the data
    if (lock_type == 'w') {
        rec.data_value += 10; // Increment the value by 10
        printf("Updating Record %d value to: %d\n", rec.id, rec.data_value);
        
        // Move pointer back to the start of the record before writing
        lseek(fd, lock.l_start, SEEK_SET);
        write(fd, &rec, sizeof(struct Record));
    }

    // 6. Hold the lock so we can test concurrency from another terminal
    printf("\nPress ENTER to release the lock and exit...\n");
    getchar();

    // 7. Release the lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    printf("Lock released.\n");

    close(fd);
    return 0;
}