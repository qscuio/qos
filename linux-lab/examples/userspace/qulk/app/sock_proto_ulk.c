#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 12345
#define MESSAGE "Hello, World!"
#define MESSAGE_COUNT 10
#define BUFFER_SIZE 1024

void hex_dump(const char *func, unsigned char *buffer, size_t length) {
    printf("-------%s-------\n", func);
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
	int i = 0;
	int sockfd;
	struct sockaddr_in6 serv_addr;
	unsigned char buffer[BUFFER_SIZE] = {0};

	if ((sockfd = socket(46, SOCK_DGRAM, 0)) < 0) {
		perror("Socket creation error");
		return -1;
	}

	send(sockfd, MESSAGE, strlen(MESSAGE), 0);
	printf("Message sent: %s\n", MESSAGE);

	while(1) {
		memset(buffer, 0, BUFFER_SIZE);
		int n = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
		if (n < 0) {
			perror("Receive error");
			close(sockfd);
			return -1;
		}
		hex_dump(__func__, buffer, n);
	}

	close(sockfd);

	return 0;
}

