#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <errno.h>

#ifdef __aarch64__
#define CHECK_ERR(x) do { if ((x) == -1) { perror(#x); exit(EXIT_FAILURE); } } while (0)

void inject_so(pid_t pid, const char *so_path) {
    struct user_pt_regs regs, saved_regs;
    long dlopen_addr, mmap_addr;
    size_t so_path_len = strlen(so_path) + 1;
    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t mmap_size = (so_path_len + page_size - 1) & ~(page_size - 1);

    // Attach to the target process
    CHECK_ERR(ptrace(PTRACE_ATTACH, pid, NULL, NULL));
    waitpid(pid, NULL, 0);

    // Get the current register values
    CHECK_ERR(ptrace(PTRACE_GETREGS, pid, NULL, &regs));
    saved_regs = regs;

    // Find the addresses of mmap and dlopen
    // These addresses should be calculated or obtained from the target process's memory maps
    dlopen_addr = (long)dlsym(RTLD_DEFAULT, "dlopen");
    mmap_addr = (long)dlsym(RTLD_DEFAULT, "mmap");

    // Set up call to mmap to allocate memory for the .so path
    regs.regs[0] = 0;                       // void *addr = 0
    regs.regs[1] = mmap_size;               // size_t length
    regs.regs[2] = PROT_READ | PROT_WRITE;  // int prot
    regs.regs[3] = MAP_PRIVATE | MAP_ANONYMOUS;  // int flags
    regs.regs[4] = -1;                      // int fd
    regs.regs[5] = 0;                       // off_t offset
    regs.pc = mmap_addr;                    // Function address

    CHECK_ERR(ptrace(PTRACE_SETREGS, pid, NULL, &regs));
    CHECK_ERR(ptrace(PTRACE_CONT, pid, NULL, NULL));
    waitpid(pid, NULL, 0);
    CHECK_ERR(ptrace(PTRACE_GETREGS, pid, NULL, &regs));
    long mmap_addr_result = regs.regs[0];

    // Write the .so path to the allocated memory
    for (size_t i = 0; i < so_path_len; i += sizeof(long)) {
        long word;
        memcpy(&word, so_path + i, sizeof(word));
        CHECK_ERR(ptrace(PTRACE_POKEDATA, pid, mmap_addr_result + i, word));
    }

    // Set up call to dlopen
    regs.regs[0] = mmap_addr_result;        // const char *filename
    regs.regs[1] = RTLD_LAZY;               // int flag
    regs.pc = dlopen_addr;                  // Function address

    CHECK_ERR(ptrace(PTRACE_SETREGS, pid, NULL, &regs));
    CHECK_ERR(ptrace(PTRACE_CONT, pid, NULL, NULL));
    waitpid(pid, NULL, 0);

    // Restore original register values
    CHECK_ERR(ptrace(PTRACE_SETREGS, pid, NULL, &saved_regs));

    // Detach from the target process
    CHECK_ERR(ptrace(PTRACE_DETACH, pid, NULL, NULL));
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <pid> <so_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t pid = atoi(argv[1]);
    const char *so_path = argv[2];

    inject_so(pid, so_path);

    return 0;
}

#endif
