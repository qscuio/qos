#include <stdio.h>
#include <ctype.h>
#include <string.h>

void dump_buffer(const unsigned char *buffer, size_t length) {
    const size_t bytes_per_line = 16;
    for (size_t i = 0; i < length; i += bytes_per_line) {
        // Print the offset
        printf("%08zx  ", i);

        // Print the hex values
        for (size_t j = 0; j < bytes_per_line; j++) {
            if (i + j < length) {
                printf("%02x ", buffer[i + j]);
            } else {
                printf("   ");
            }
        }

        // Print a separator
        printf(" |");

        // Print the ASCII values
        for (size_t j = 0; j < bytes_per_line; j++) {
            if (i + j < length) {
                unsigned char c = buffer[i + j];
                printf("%c", isprint(c) ? c : '.');
            } else {
                printf(" ");
            }
        }

        // End of line
        printf("|\n");
    }
}

int main() {
    const char *test_data = "This is a test buffer. It contains some sample text.";
    dump_buffer((const unsigned char *)test_data, strlen(test_data));
    return 0;
}

