// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to send the entire message in parts
void send_message_in_parts(int sock_fd, const char *message, size_t message_length) {
    uint32_t net_message_length = htonl(message_length); // Convert to network byte order

    // Send the message length first (4-byte header)
    if (send(sock_fd, &net_message_length, sizeof(net_message_length), 0) < 0) {
        perror("Send message length failed");
        return;
    }

    // Simulate partial sends by sending the message in chunks
    size_t total_sent = 0;
    size_t chunk_size = 5; // Send 5 bytes at a time
    while (total_sent < message_length) {
        size_t bytes_to_send = (message_length - total_sent > chunk_size) ? chunk_size : message_length - total_sent;
        ssize_t bytes_sent = send(sock_fd, message + total_sent, bytes_to_send, 0);
        if (bytes_sent < 0) {
            perror("Send message failed");
            return;
        }
        total_sent += bytes_sent;
        printf("Sent %zd bytes of message...\n", bytes_sent);
    }
}

int main() {
    int sock_fd;
    struct sockaddr_in server_address;

    // Create the socket
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // Message to send
    const char *message = "Hello, Server! This message is sent in parts.";
    size_t message_length = strlen(message);

    // Send the message in parts
    send_message_in_parts(sock_fd, message, message_length);

    // Close the socket
    close(sock_fd);
    return 0;
}

