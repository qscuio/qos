#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("kprobe/do_sys_open")
int kprobe_file_open(struct pt_regs *ctx) {
    int fd = PT_REGS_PARM1(ctx);
    bpf_printk("File opened with descriptor: %d\n", fd);
    return 0;
}

char _license[] SEC("license") = "GPL";

