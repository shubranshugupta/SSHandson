#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // a. Ensure input is taken from the command line
    if (argc != 2) {
        printf("Usage: ./a.out <filename_or_path>\n");
        return 1;
    }

    struct stat file_stat;

    // Use lstat() to correctly handle symbolic links
    if (lstat(argv[1], &file_stat) == -1) {
        perror("Error retrieving file information");
        return 1;
    }

    printf("File: '%s'\n", argv[1]);
    printf("Type: ");

    // b. Identify any type of file using the st_mode field
    if (S_ISREG(file_stat.st_mode)) {
        printf("Regular File\n");
    } 
    else if (S_ISDIR(file_stat.st_mode)) {
        printf("Directory\n");
    } 
    else if (S_ISCHR(file_stat.st_mode)) {
        printf("Character Device (e.g., terminal, keyboard)\n");
    } 
    else if (S_ISBLK(file_stat.st_mode)) {
        printf("Block Device (e.g., hard drive, USB)\n");
    } 
    else if (S_ISFIFO(file_stat.st_mode)) {
        printf("FIFO / Named Pipe\n");
    } 
    else if (S_ISLNK(file_stat.st_mode)) {
        printf("Symbolic Link (Soft Link)\n");
    } 
    else if (S_ISSOCK(file_stat.st_mode)) {
        printf("Unix Domain Socket\n");
    } 
    else {
        printf("Unknown File Type\n");
    }

    return 0;
}