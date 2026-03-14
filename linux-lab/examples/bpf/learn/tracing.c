#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("kprobe/sys_clone")
int kprobe_example(struct pt_regs *ctx) {
    bpf_printk("sys_clone() called\n"); // Print when the sys_clone syscall is invoked
    return 0;
}

char _license[] SEC("license") = "GPL";

