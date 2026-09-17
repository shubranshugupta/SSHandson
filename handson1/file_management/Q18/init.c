#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

// Define a fixed-size record structure
struct Record {
    int id;
    int data_value;
};

int main() {
    int fd = open("database.dat", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error creating file");
        return 1;
    }

    // Create and write 5 records to the file
    for (int i = 1; i <= 5; i++) {
        struct Record rec;
        rec.id = i;
        rec.data_value = i * 100; // E.g., Record 1 = 100, Record 2 = 200...
        
        write(fd, &rec, sizeof(struct Record));
    }

    printf("Successfully initialized 'database.dat' with 5 records.\n");
    
    close(fd);
    return 0;
}