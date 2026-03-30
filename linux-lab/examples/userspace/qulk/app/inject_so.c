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

/*
 * Steps to Inject an .so File using ptrace
 * * Attach to the Target Process: Use ptrace(PTRACE_ATTACH, pid, NULL, NULL) to attach to the target process.
 * * Find the Address of dlopen: Locate the address of dlopen in the target process. This often involves parsing the target process's memory maps.
 * * Allocate Memory in the Target Process: Use mmap to allocate memory in the target process for the .so file path string.
 * * Write the .so File Path into the Target Process's Memory: Use ptrace(PTRACE_POKETEXT, ...) to write the path of the .so file into the allocated memory.
 * * Call dlopen in the Target Process: Use ptrace to set up and execute a call to dlopen with the path of the .so file.
 * * Detach from the Target Process: Use ptrace(PTRACE_DETACH, pid, NULL, NULL) to detach from the target process.
 */

#define CHECK_ERR(x) do { if ((x) == -1) { perror(#x); exit(EXIT_FAILURE); } } while (0)

void inject_so(pid_t pid, const char *so_path) {
    struct user_regs_struct regs, saved_regs;
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
    regs.rax = mmap_addr;
    regs.rdi = 0;
    regs.rsi = mmap_size;
    regs.rdx = PROT_READ | PROT_WRITE;
    regs.r10 = MAP_PRIVATE | MAP_ANONYMOUS;
    regs.r8 = -1;
    regs.r9 = 0;
    CHECK_ERR(ptrace(PTRACE_SETREGS, pid, NULL, &regs));
    CHECK_ERR(ptrace(PTRACE_CONT, pid, NULL, NULL));
    waitpid(pid, NULL, 0);
    CHECK_ERR(ptrace(PTRACE_GETREGS, pid, NULL, &regs));
    long mmap_addr_result = regs.rax;

    // Write the .so path to the allocated memory
    for (size_t i = 0; i < so_path_len; i += sizeof(long)) {
        long word;
        memcpy(&word, so_path + i, sizeof(word));
        CHECK_ERR(ptrace(PTRACE_POKETEXT, pid, mmap_addr_result + i, word));
    }

    // Set up call to dlopen
    regs.rax = dlopen_addr;
    regs.rdi = mmap_addr_result;
    regs.rsi = RTLD_LAZY;
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

