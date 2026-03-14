#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/if_ether.h>
#include <linux/ip.h>

SEC("xdp")
int xdp_packet_filter(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end) return XDP_PASS; // Invalid packet size

    if (eth->h_proto == bpf_htons(ETH_P_IP)) {
        struct iphdr *iph = data + sizeof(*eth);
        if ((void *)(iph + 1) > data_end) return XDP_PASS;

        // Check if the source IP is 192.168.1.1
        if (iph->saddr == bpf_htonl(0xC0A80101)) {
            return XDP_DROP; // Drop packet if condition matches
        }
    }

    return XDP_PASS; // Let other packets through
}

char _license[] SEC("license") = "GPL";

