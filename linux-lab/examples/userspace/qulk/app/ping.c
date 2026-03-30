#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>

#define PACKET_SIZE 64
#define PING_COUNT 4
#define TIMEOUT 1

// Checksum function
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

// Helper function to get the current time in milliseconds
long get_time_in_ms() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (time.tv_sec * 1000) + (time.tv_usec / 1000);
}

// Global variable to handle interrupt signal
volatile int pingloop = 1;

// Signal handler for interrupt signal (Ctrl+C)
void intHandler(int dummy) {
    pingloop = 0;
}

// Main function
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *hostname = argv[1];
    int sockfd;
    struct addrinfo hints, *res;
    struct sockaddr_in dest_addr;
    struct icmp icmp_pkt;
    char send_packet[PACKET_SIZE];
    char recv_packet[PACKET_SIZE];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    struct timeval timeout;
    int sequence = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;

    if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    dest_addr = *(struct sockaddr_in *)res->ai_addr;
    freeaddrinfo(res);

    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    printf("PING %s (%s) %d bytes of data.\n", hostname, inet_ntoa(dest_addr.sin_addr), PACKET_SIZE);

    signal(SIGINT, intHandler);

    while (pingloop && sequence <= PING_COUNT) {
        memset(&icmp_pkt, 0, sizeof(icmp_pkt));
        icmp_pkt.icmp_type = ICMP_ECHO;
        icmp_pkt.icmp_code = 0;
        icmp_pkt.icmp_id = getpid();
        icmp_pkt.icmp_seq = sequence++;
        icmp_pkt.icmp_cksum = checksum(&icmp_pkt, sizeof(icmp_pkt));

        memset(send_packet, 0, PACKET_SIZE);
        memcpy(send_packet, &icmp_pkt, sizeof(icmp_pkt));

        long start_time = get_time_in_ms();
        if (sendto(sockfd, send_packet, PACKET_SIZE, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("sendto");
            exit(EXIT_FAILURE);
        }

        if (recvfrom(sockfd, recv_packet, PACKET_SIZE, 0, (struct sockaddr *)&from, &fromlen) < 0) {
            if (errno == EAGAIN) {
                printf("Request timeout for icmp_seq %d\n", sequence - 1);
            } else {
                perror("recvfrom");
                exit(EXIT_FAILURE);
            }
        } else {
            long end_time = get_time_in_ms();
            struct iphdr *ip_hdr = (struct iphdr *)recv_packet;
            struct icmphdr *icmp_hdr = (struct icmphdr *)(recv_packet + (ip_hdr->ihl * 4));

            if (icmp_hdr->type == ICMP_ECHOREPLY) {
                printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%ld ms\n", PACKET_SIZE, inet_ntoa(from.sin_addr), sequence - 1, ip_hdr->ttl, end_time - start_time);
            } else {
                printf("Error: ICMP type %d code %d\n", icmp_hdr->type, icmp_hdr->code);
            }
        }

        sleep(1);
    }

    close(sockfd);
    return 0;
}

