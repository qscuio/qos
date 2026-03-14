#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("xdp")
int xdp_prog_example(struct xdp_md *ctx) {
    // Example XDP program that simply drops every packet
    return XDP_DROP;
}

// Required license declaration for eBPF programs
char _license[] SEC("license") = "GPL";

