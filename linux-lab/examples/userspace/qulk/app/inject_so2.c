#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>


// Function to find the base address of a module in the target process
void* get_module_base(pid_t pid, const char* module_name) {
    char filename[256];
    FILE *fp;
    char line[256];
    void *base_addr = NULL;

    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        return NULL;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, module_name)) {
            sscanf(line, "%p-%*p %*s %*s %*s %*s %*s", &base_addr);
            break;
        }
    }

    fclose(fp);

    return base_addr;
}

// Function to calculate the remote address of a symbol in the target process
void* get_remote_addr(pid_t pid, const char* module_name, void* local_addr) {
    void *local_base, *remote_base;

    local_base = get_module_base(getpid(), module_name);
    remote_base = get_module_base(pid, module_name);

    if (local_base == NULL || remote_base == NULL) {
        return NULL;
    }

    return (void *)((uintptr_t)local_addr - (uintptr_t)local_base + (uintptr_t)remote_base);
}

// Function to read the return value of a function call
void read_return_value(pid_t pid, long *result) {
    struct user_regs_struct regs;

    // Get the registers from the target process
    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    *result = regs.rax;  // On x86_64, the return value is in the RAX register
}

void inject_so(pid_t pid, const char* so_path) {
    struct user_regs_struct regs, original_regs;
    void* dlopen_addr;
    void* remote_dlopen_addr;
    char *lib_path;
    int status;
    long dlopen_result;

    // Attach to the process
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
        perror("PTRACE_ATTACH");
        exit(1);
    }
    
    waitpid(pid, &status, 0);

    // Save current register state
    ptrace(PTRACE_GETREGS, pid, NULL, &original_regs);
    memcpy(&regs, &original_regs, sizeof(struct user_regs_struct));

    // Allocate memory in the target process for the library path
    lib_path = (char *)regs.rsp - 256; // Adjust according to available stack space
    for (size_t i = 0; i < strlen(so_path); i += sizeof(long)) {
        ptrace(PTRACE_POKEDATA, pid, lib_path + i, *(long *)(so_path + i));
    }
    ptrace(PTRACE_POKEDATA, pid, lib_path + strlen(so_path), 0); // Null-terminate the string

    // Find the address of dlopen in the target process
    dlopen_addr = dlsym(RTLD_DEFAULT, "dlopen");
    if (!dlopen_addr) {
        fprintf(stderr, "Cannot find dlopen address\n");
        exit(1);
    }

    printf("local dlopen addr %p\n", dlopen_addr);
    remote_dlopen_addr = get_remote_addr(pid, "libc", dlopen_addr);
    if (!remote_dlopen_addr) {
        fprintf(stderr, "Cannot find remote dlopen address\n");
        exit(1);
    }
    printf("remote dlopen addr %p\n", remote_dlopen_addr);

   // Set up parameters for dlopen in registers
    regs.rdi = (long)lib_path; // First parameter: path to the .so file
    regs.rsi = RTLD_NOW; // Second parameter: flags

    // Set the instruction pointer to dlopen and update registers
    regs.rip = (long)remote_dlopen_addr;
    ptrace(PTRACE_SETREGS, pid, NULL, &regs);
    ptrace(PTRACE_CONT, pid, NULL, NULL);


    waitpid(pid, &status, 0);

    // Read the return value from the target process
    read_return_value(pid, &dlopen_result);

    printf("dlopen returned: %p\n", (void*)dlopen_result);

    // Restore original register state
    ptrace(PTRACE_SETREGS, pid, NULL, &original_regs);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <pid> <so_path>\n", argv[0]);
        return 1;
    }

    pid_t pid = atoi(argv[1]);
    const char* so_path = argv[2];

    inject_so(pid, so_path);

    return 0;
}

