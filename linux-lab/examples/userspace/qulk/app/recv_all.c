// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define HEADER_SIZE 4
#define BUFFER_SIZE 1024

// Function to read exactly 'n' bytes from the socket
ssize_t recv_all(int sock_fd, char *buffer, size_t length) {
    ssize_t total_received = 0;
    while (total_received < length) {
        ssize_t bytes_received = recv(sock_fd, buffer + total_received, length - total_received, 0);
        if (bytes_received <= 0) {
            return bytes_received; // Error or connection closed
        }
        total_received += bytes_received;
    }
    return total_received;
}

// Function to handle receiving a complete message with a header
void receive_complete_message(int sock_fd) {
    char header[HEADER_SIZE];
    char *message_buffer;
    ssize_t bytes_received;

    // First, read the fixed-length header
    bytes_received = recv_all(sock_fd, header, HEADER_SIZE);
    if (bytes_received <= 0) {
        perror("Failed to read header");
        return;
    }

    // Convert header to message length (assuming it's a 4-byte integer)
    uint32_t message_length;
    memcpy(&message_length, header, sizeof(uint32_t));
    message_length = ntohl(message_length);  // Convert from network byte order to host byte order

    // Allocate memory for the message
    message_buffer = (char *)malloc(message_length);
    if (message_buffer == NULL) {
        perror("Memory allocation failed");
        return;
    }

    // Now read the complete message
    bytes_received = recv_all(sock_fd, message_buffer, message_length);
    if (bytes_received <= 0) {
        perror("Failed to read message");
        free(message_buffer);
        return;
    }

    // Process the message (just printing here)
    printf("Received message: %.*s\n", message_length, message_buffer);

    // Free the buffer after use
    free(message_buffer);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create the socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept a connection
    if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Receive and process the complete message
    receive_complete_message(client_fd);

    // Close sockets
    close(client_fd);
    close(server_fd);
    return 0;
}

