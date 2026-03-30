#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <netinet/ether.h>
#include <time.h>
#include <errno.h>
#include <poll.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_PORTS 10
#define MAC_TABLE_SIZE 100
#define MAC_TIMEOUT 300  // Timeout for MAC address in seconds
#define VLAN_HEADER_LEN 4

typedef struct {
    unsigned char mac[ETH_ALEN];  // MAC address
    int port;                     // Port number
    time_t timestamp;             // Time when the MAC was learned
    int vlan_id;                  // VLAN ID associated with the MAC address
} mac_entry_t;

typedef struct {
    int packets_received;         // Number of packets received
    int packets_forwarded;        // Number of packets forwarded
    int packets_flooded;          // Number of packets flooded
    int vlan_id;                  // VLAN ID assigned to the port
} port_stats_t;

// MAC table for learning
mac_entry_t mac_table[MAC_TABLE_SIZE];
int mac_table_size = 0;

// Pointer to shared port statistics (will be shared using mmap)
port_stats_t *port_stats;

// Function to create a TAP interface
int create_tap_interface(char *dev, int flags) {
    struct ifreq ifr;
    int fd, err;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("Opening /dev/net/tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = flags;  // IFF_TUN for TUN device, IFF_TAP for TAP device
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return err;
    }

    printf("TAP interface %s created\n", ifr.ifr_name);
    return fd;
}

// Function to bring the TAP interface up
int bring_up_interface(char *dev) {
    struct ifreq ifr;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    // Get interface flags, add the IFF_UP flag, and set the interface flags
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
        perror("ioctl(SIOCGIFFLAGS)");
        close(sockfd);
        return -1;
    }

    ifr.ifr_flags |= IFF_UP;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl(SIOCSIFFLAGS)");
        close(sockfd);
        return -1;
    }

    close(sockfd);
    printf("Interface %s is up\n", dev);
    return 0;
}

// Function to delete TAP interface (bring it down first)
int delete_tap_interface(char *dev) {
    struct ifreq ifr;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    // Bring the interface down
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
        perror("ioctl(SIOCGIFFLAGS)");
        close(sockfd);
        return -1;
    }

    ifr.ifr_flags &= ~IFF_UP;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl(SIOCSIFFLAGS)");
        close(sockfd);
        return -1;
    }

    printf("Interface %s brought down\n", dev);

    close(sockfd);
    printf("TAP interface %s deleted\n", dev);
    return 0;
}

// Find MAC in the table
int find_mac(unsigned char *mac, int vlan_id) {
    for (int i = 0; i < mac_table_size; i++) {
        if (memcmp(mac_table[i].mac, mac, ETH_ALEN) == 0 && mac_table[i].vlan_id == vlan_id) {
            return i;
        }
    }
    return -1;
}

// Add MAC to the table
void add_mac(unsigned char *mac, int port, int vlan_id) {
    int index = find_mac(mac, vlan_id);
    if (index != -1) {
        // Update existing entry
        mac_table[index].port = port;
        mac_table[index].timestamp = time(NULL);
    } else {
        // Add new entry
        if (mac_table_size < MAC_TABLE_SIZE) {
            memcpy(mac_table[mac_table_size].mac, mac, ETH_ALEN);
            mac_table[mac_table_size].port = port;
            mac_table[mac_table_size].timestamp = time(NULL);
            mac_table[mac_table_size].vlan_id = vlan_id;
            mac_table_size++;
        }
    }
}

// Remove stale MAC addresses from the table
void clean_mac_table() {
    time_t current_time = time(NULL);
    for (int i = 0; i < mac_table_size; i++) {
        if (difftime(current_time, mac_table[i].timestamp) > MAC_TIMEOUT) {
            // Remove the entry
            for (int j = i; j < mac_table_size - 1; j++) {
                mac_table[j] = mac_table[j + 1];
            }
            mac_table_size--;
            i--;
        }
    }
}

// Log packet details
void log_packet(const unsigned char *src_mac, const unsigned char *dst_mac, int recv_port, int action_port, int vlan_id) {
    char src_str[18], dst_str[18];
    snprintf(src_str, sizeof(src_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);
    snprintf(dst_str, sizeof(dst_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             dst_mac[0], dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5]);

    if (action_port == -1) {
        printf("VLAN %d, Port %d: Received packet from %s to %s (Flooding)\n", vlan_id, recv_port, src_str, dst_str);
    } else {
        printf("VLAN %d, Port %d: Received packet from %s to %s (Forwarding to port %d)\n", vlan_id, recv_port, src_str, dst_str, action_port);
    }
}

// Extract VLAN ID from a VLAN-tagged packet (802.1Q)
int extract_vlan_id(unsigned char *packet, int *is_vlan_tagged) {
    struct ether_header *eth_hdr = (struct ether_header *)packet;
    if (ntohs(eth_hdr->ether_type) == 0x8100) {  // Check for VLAN tag (0x8100)
        *is_vlan_tagged = 1;
        unsigned char *vlan_header = packet + sizeof(struct ether_header);
        int vlan_id = ((vlan_header[0] & 0x0F) << 8) | vlan_header[1];  // Extract VLAN ID from VLAN header
        return vlan_id;
    }
    *is_vlan_tagged = 0;
    return -1;
}

// Packet forwarding function
void forward_packet(int recv_port, unsigned char *packet, int len, int fds[]) {
    struct ether_header *eth_hdr = (struct ether_header *)packet;

    // Extract VLAN ID
    int is_vlan_tagged;
    int vlan_id = extract_vlan_id(packet, &is_vlan_tagged);
    if (!is_vlan_tagged) {
        vlan_id = port_stats[recv_port].vlan_id;  // Assign untagged packets to the port's VLAN
    }

    // Increment packet receive counter for the port
    port_stats[recv_port].packets_received++;

    // Learn the source MAC address (with VLAN)
    add_mac(eth_hdr->ether_shost, recv_port, vlan_id);

    // Find the destination MAC in the MAC table (within the same VLAN)
    int dest_port = find_mac(eth_hdr->ether_dhost, vlan_id);

    if (dest_port == -1) {
        // Unknown destination, flood to all ports within the same VLAN except recv_port
        log_packet(eth_hdr->ether_shost, eth_hdr->ether_dhost, recv_port, -1, vlan_id);
        for (int i = 0; i < MAX_PORTS; i++) {
            if (i != recv_port && port_stats[i].vlan_id == vlan_id) {
                write(fds[i], packet, len);
                port_stats[i].packets_flooded++;
            }
        }
        port_stats[recv_port].packets_flooded++;
    } else {
        // Forward to the known port
        log_packet(eth_hdr->ether_shost, eth_hdr->ether_dhost, recv_port, mac_table[dest_port].port, vlan_id);
        write(fds[mac_table[dest_port].port], packet, len);
        port_stats[mac_table[dest_port].port].packets_forwarded++;
    }
}

// Dynamically set VLAN ID for a port
void set_port_vlan_id(int port, int vlan_id) {
    if (port < 0 || port >= MAX_PORTS) {
        printf("Invalid port number: %d\n", port);
        return;
    }

    printf("Setting VLAN ID %d for port %d\n", vlan_id, port);
    port_stats[port].vlan_id = vlan_id;
}

// Command-line interface to dynamically configure VLANs
void command_interface() {
    char command[256];
    int port, vlan_id;

    while (1) {
        printf("Enter command to set VLAN (format: set_vlan <port> <vlan_id>): ");
        fgets(command, sizeof(command), stdin);

        if (sscanf(command, "set_vlan %d %d", &port, &vlan_id) == 2) {
            set_port_vlan_id(port, vlan_id);
        } else {
            printf("Invalid command format. Use: set_vlan <port> <vlan_id>\n");
        }
    }
}

int main() {
    char ifname[MAX_PORTS][IFNAMSIZ];
    int fds[MAX_PORTS];
    struct pollfd poll_fds[MAX_PORTS];

    // Allocate shared memory for port statistics
    port_stats = mmap(NULL, sizeof(port_stats_t) * MAX_PORTS, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (port_stats == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Step 1: Create TAP interfaces and bring them up
    for (int i = 0; i < MAX_PORTS; i++) {
        snprintf(ifname[i], IFNAMSIZ, "tap%d", i);
        fds[i] = create_tap_interface(ifname[i], IFF_TAP | IFF_NO_PI);
        if (fds[i] < 0) {
            fprintf(stderr, "Error creating TAP interface %s\n", ifname[i]);
            exit(1);
        }

        if (bring_up_interface(ifname[i]) < 0) {
            fprintf(stderr, "Error bringing up TAP interface %s\n", ifname[i]);
            exit(1);
        }

        poll_fds[i].fd = fds[i];
        poll_fds[i].events = POLLIN;

        // Initialize per-port statistics and default VLANs
        port_stats[i].packets_received = 0;
        port_stats[i].packets_forwarded = 0;
        port_stats[i].packets_flooded = 0;
        port_stats[i].vlan_id = 100;  // Set a default VLAN (e.g., VLAN 100)
    }

    // Run the command interface in a separate process
    if (fork() == 0) {
        command_interface();  // Command interface in child process
        exit(0);
    }

    unsigned char buffer[ETH_FRAME_LEN];
    while (1) {
        // Clean MAC table periodically
        clean_mac_table();

        // Poll for packets on each port
        int ret = poll(poll_fds, MAX_PORTS, -1);
        if (ret < 0) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < MAX_PORTS; i++) {
            if (poll_fds[i].revents & POLLIN) {
                int len = read(fds[i], buffer, ETH_FRAME_LEN);
                if (len > 0) {
                    forward_packet(i, buffer, len, fds);
                }
            }
        }
    }

    // Step 2: Clean up and delete the TAP interfaces (never reached in this infinite loop)
    for (int i = 0; i < MAX_PORTS; i++) {
        delete_tap_interface(ifname[i]);
        close(fds[i]);
    }

    return 0;
}

