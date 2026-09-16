#include <fcntl.h>
#include <unistd.h>

#define MAX_LINE_LEN 1024

int main(int argc, char *argv[]) {
    // 1. Ensure the user provided a filename
    if (argc != 2) {
        write(2, "Usage: ./read_lines <filename>\n", 31);
        return 1;
    }

    // 2. Open the file in Read-Only mode
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        write(2, "Error: Could not open file\n", 27);
        return 1;
    }

    char buffer[MAX_LINE_LEN];
    int index = 0;
    char current_char;
    ssize_t bytes_read;

    // 3. Read loop: Fetch 1 byte at a time from the file
    while ((bytes_read = read(fd, &current_char, 1)) > 0) {
        buffer[index++] = current_char;
        
        // If we hit a newline OR fill our buffer max, flush the line to STDOUT
        if (current_char == '\n' || index == MAX_LINE_LEN) {
            write(1, buffer, index);
            index = 0; // Reset the index for the next line
        }
    }

    // 4. Handle any remaining characters if the file doesn't end with a newline
    if (index > 0) {
        write(1, buffer, index);
        write(1, "\n", 1); // Add a final newline for clean terminal output
    }

    if (bytes_read == -1) {
        write(2, "Error: Reading file failed\n", 27);
    }

    // 5. Close the file upon reaching EOF
    close(fd);

    return 0;
}