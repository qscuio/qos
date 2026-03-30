#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define BUFFER_SIZE 1024

// Telnet 协议定义
#define IAC  255  // Interpret As Command
#define DONT 254  // 拒绝选项
#define DO   253  // 请求对方启用选项
#define WONT 252  // 拒绝启用选项
#define WILL 251  // 同意启用选项
#define SB   250  // Subnegotiation Begin
#define SE   240  // Subnegotiation End

// 处理 Telnet 选项协商
void handle_telnet_option(int sockfd, unsigned char *buffer, int len) {
    for (int i = 0; i < len; i++) {
        if (buffer[i] == IAC) {
            if (i + 1 >= len) break; // 防止越界

            unsigned char command = buffer[i + 1];
            unsigned char option = buffer[i + 2];

            switch (command) {
                case DO:
                    printf("Server requested DO option: %d\n", option);
                    // 默认拒绝所有选项
                    write(sockfd, (unsigned char[]){IAC, WONT, option}, 3);
                    break;
                case DONT:
                    printf("Server requested DONT option: %d\n", option);
                    // 默认同意 DONT
                    write(sockfd, (unsigned char[]){IAC, WONT, option}, 3);
                    break;
                case WILL:
                    printf("Server requested WILL option: %d\n", option);
                    // 默认拒绝所有选项
                    write(sockfd, (unsigned char[]){IAC, DONT, option}, 3);
                    break;
                case WONT:
                    printf("Server requested WONT option: %d\n", option);
                    // 默认同意 WONT
                    write(sockfd, (unsigned char[]){IAC, DONT, option}, 3);
                    break;
                case SB:
                    // 子选项协商，跳过处理
                    printf("Subnegotiation received, skipping...\n");
                    while (i < len && buffer[i] != SE) i++;
                    break;
                default:
                    printf("Unknown Telnet command: %d\n", command);
                    break;
            }
            i += 2; // 跳过命令和选项字节
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *hostname = argv[1];
    int port = atoi(argv[2]);

    int sockfd;
    struct sockaddr_in server_addr;

    // 创建 socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // 将主机名转换为 IP 地址
    if (inet_pton(AF_INET, hostname, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 连接到服务器
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to %s:%d\n", hostname, port);

    char buffer[BUFFER_SIZE];
    fd_set read_fds;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        // 等待输入或服务器数据
        if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            break;
        }

        // 从标准输入读取数据并发送到服务器
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            memset(buffer, 0, BUFFER_SIZE);
            int n = read(STDIN_FILENO, buffer, BUFFER_SIZE);
            if (n <= 0) {
                break;
            }
            if (write(sockfd, buffer, n) < 0) {
                perror("Write to socket failed");
                break;
            }
        }

        // 从服务器读取数据并输出到标准输出
        if (FD_ISSET(sockfd, &read_fds)) {
            memset(buffer, 0, BUFFER_SIZE);
            int n = read(sockfd, buffer, BUFFER_SIZE);
            if (n <= 0) {
                printf("Connection closed by server\n");
                break;
            }

            // 检查是否有 Telnet 选项协商
            handle_telnet_option(sockfd, (unsigned char *)buffer, n);

            // 输出非选项协商的数据
            for (int i = 0; i < n; i++) {
                if (buffer[i] != IAC) {
                    putchar(buffer[i]);
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
