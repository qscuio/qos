#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/user.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#if 0
const char shellcode[] = 
    "\x48\x31\xc0"                  // xor    %rax,%rax
    "\x48\xc7\xc0\x09\x00\x00\x00"  // mov    $0x9,%rax (syscall number for mmap)
    "\x48\xc7\xc7\x00\x00\x00\x00"  // mov    $0x0,%rdi (addr)
    "\x48\xc7\xc6\x00\x10\x00\x00"  // mov    $0x1000,%rsi (length)
    "\x48\xc7\xc2\x07\x00\x00\x00"  // mov    $0x7,%rdx (prot)
    "\x48\xc7\xc1\x22\x00\x00\x00"  // mov    $0x22,%rcx (flags)
    "\x48\xc7\xc0\xff\xff\xff\xff"  // mov    $-1,%r8  (fd)
    "\x48\xc7\xc0\x00\x00\x00\x00"  // mov    $0x0,%r9  (offset)
    "\x0f\x05"                      // syscall
    "\xcc";                         // int3 (breakpoint)
#else
const char shellcode[] = {0xf3,0x0f,0x1e,0xfa,0x55,0x48,0x89,0xe5,0x48,0x8d,0x05,0xc9,0x0e,0x00,0x00,0x48,0x89,0xc7,0xb8,0x00,0x00,0x00,0x00,0xe8,0x0c,0xff,0xff,0xff,0xb8,0x00,0x00,0x00,0x00,0x5d,0xc3 };
#endif


void inject_code(pid_t pid, unsigned long addr, const void *code, size_t size) {
    for (size_t i = 0; i < size; i += sizeof(long)) {
        unsigned long data = 0;
        memcpy(&data, code + i, sizeof(long));
        ptrace(PTRACE_POKETEXT, pid, addr + i, data);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }

    pid_t pid = atoi(argv[1]);
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
        perror("ptrace attach");
        return 1;
    }

    waitpid(pid, NULL, 0);

    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace getregs");
        return 1;
    }

    unsigned long shellcode_addr = regs.rip;  // or another address you determine to be safe
    inject_code(pid, shellcode_addr, shellcode, sizeof(shellcode));

    regs.rip = shellcode_addr;
    if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace setregs");
        return 1;
    }

    if (ptrace(PTRACE_CONT, pid, NULL, NULL) == -1) {
        perror("ptrace cont");
        return 1;
    }

    waitpid(pid, NULL, 0);

    if (ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1) {
        perror("ptrace detach");
        return 1;
    }

    return 0;
}

