#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#define NETLINK_USER 31  // Same Netlink family ID as in kernel module
#define MSG_LEN 128      // Maximum message length

int main() {
    struct sockaddr_nl src_addr, dest_addr;
    struct nlmsghdr *nlh;
    struct iovec iov;
    struct msghdr msg;
    int sock_fd, retval;
    char message[MSG_LEN] = "Hello from user space!";

    // Create a Netlink socket
    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (sock_fd < 0) {
        perror("Failed to create socket");
        return 1;
    }

    // Bind the socket
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();  // Use unique PID for the socket
    src_addr.nl_groups = 0;      // Unicast

    if (bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
        perror("Failed to bind socket");
        close(sock_fd);
        return 1;
    }

    // Prepare message to kernel
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;  // Kernel PID (0 for kernel)
    dest_addr.nl_groups = 0;  // Unicast

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MSG_LEN));
    if (!nlh) {
        perror("Failed to allocate memory for message");
        close(sock_fd);
        return 1;
    }

    memset(nlh, 0, NLMSG_SPACE(MSG_LEN));
    nlh->nlmsg_len = NLMSG_SPACE(MSG_LEN);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;
    strncpy(NLMSG_DATA(nlh), message, strlen(message));

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    // Send message to kernel
    retval = sendmsg(sock_fd, &msg, 0);
    if (retval < 0) {
        perror("Failed to send message to kernel");
    } else {
        printf("Message sent to kernel: %s\n", message);
    }

    // Receive message from kernel
    memset(nlh, 0, NLMSG_SPACE(MSG_LEN));
    retval = recvmsg(sock_fd, &msg, 0);
    if (retval < 0) {
        perror("Failed to receive message from kernel");
    } else {
        printf("Message received from kernel: %s\n", (char *)NLMSG_DATA(nlh));
    }

    // Cleanup
    free(nlh);
    close(sock_fd);

    return 0;
}

