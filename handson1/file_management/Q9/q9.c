#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // 1. Ensure the user provided a filename
    if (argc != 2) {
        write(2, "Usage: ./a.out <filename>\n", 30);
        return 1;
    }

    struct stat file_stat;

    // 2. The stat() system call
    // It takes the filename and the memory address of our stat structure
    if (stat(argv[1], &file_stat) == -1) {
        perror("Error retrieving file status");
        return 1;
    }

    // 3. Print the requested data
    // Variables are cast to standard types (unsigned long, long long, etc.) 
    // to prevent format string warnings across different 32/64-bit architectures.
    
    printf("========== Information for: %s ==========\n", argv[1]);
    
    printf("a. Inode: \t\t\t%lu\n", (unsigned long)file_stat.st_ino);
    printf("b. Number of hard links: \t%lu\n", (unsigned long)file_stat.st_nlink);
    printf("c. UID (User ID): \t\t%u\n", (unsigned int)file_stat.st_uid);
    printf("d. GID (Group ID): \t\t%u\n", (unsigned int)file_stat.st_gid);
    printf("e. Size: \t\t\t%lld bytes\n", (long long)file_stat.st_size);
    printf("f. Block size: \t\t\t%ld bytes\n", (long)file_stat.st_blksize);
    printf("g. Number of blocks: \t\t%lld\n", (long long)file_stat.st_blocks);
    
    // ctime() converts the raw seconds (time_t) into a readable human string.
    // Note: ctime() automatically appends a newline (\n) to its output.
    printf("h. Time of last access: \t%s", ctime(&file_stat.st_atime));
    printf("i. Time of last modification: \t%s", ctime(&file_stat.st_mtime));
    printf("j. Time of last change: \t%s", ctime(&file_stat.st_ctime));

    return 0;
}