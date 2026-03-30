#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/filter.h>

static int sock_open(const struct sock_fprog *fprog)
{
    int s;

    s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (s < 0) {
        perror("socket");
        return -1;
    }

    if (fprog) {
        if (setsockopt(s, SOL_SOCKET, SO_ATTACH_FILTER,
                       fprog, sizeof(*fprog)) < 0) {
            perror("setsockopt(SO_ATTACH_FILTER)");
            close(s);
            return -1;
        }
    }

    return s;
}

int main(void)
{
    /* 只接收 IPv4 的 classic BPF */
    struct sock_filter code[] = {
        /* load half-word at offset 12 (EtherType) */
        BPF_STMT(BPF_LD  | BPF_H | BPF_ABS, 12),
        /* if == ETH_P_IP jump to accept */
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ETH_P_IP, 0, 1),
        /* accept packet */
        BPF_STMT(BPF_RET | BPF_K, 0xFFFFFFFF),
        /* reject packet */
        BPF_STMT(BPF_RET | BPF_K, 0),
    };

    struct sock_fprog fprog = {
        .len = sizeof(code) / sizeof(code[0]),
        .filter = code,
    };

    int fd = sock_open(&fprog);
    if (fd < 0) {
        fprintf(stderr, "failed to open socket\n");
        return 1;
    }

    printf("listening...\n");

    while (1) {
        unsigned char buf[2048];
        ssize_t n = recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL);
        if (n < 0) {
            perror("recvfrom");
            break;
        }
        printf("got packet: %zd bytes\n", n);
    }

    close(fd);
    return 0;
}

