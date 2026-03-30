#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>
#include <sys/mman.h>
#include <string.h>

#define __TRACE() printf("[%s][%d]---------\n", __func__, __LINE__)

// Define the code to inject: sets eax to 42 and then triggers a breakpoint
unsigned char injected_code[] = {
    0x00, 0x2a, 0x00, 0x00, 0x00, // mov eax, 42
    0xcc                          // int3 (breakpoint to stop execution)
};

#if 0
unsigned char injected_code[] = {
    0xb8, 0x2a, 0x00, 0x00, 0x00, // mov eax, 42
    0xcc                          // int3 (breakpoint to stop execution)
};
#endif

#define CODE_SIZE (sizeof(injected_code))

void die(const char *msg) {
    perror(msg);
    exit(1);
}

// Function to get the entry point from the target process
unsigned long get_entry_point(pid_t pid) {
    char filename[BUFSIZ];
    sprintf(filename, "/proc/%d/exe", (int)pid);

    const size_t length = 0x1000;

    int fd = open(filename, O_RDONLY);
    if (fd < 0) die("open");

    void *addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) die("mmap");

    Elf64_Ehdr *header = addr;
    unsigned long entry_point = header->e_entry;

    if (header->e_type == ET_DYN) {
        sprintf(filename, "/proc/%d/maps", (int)pid);
        int mapsfd = open(filename, O_RDONLY);
        if (mapsfd < 0) die("open maps");

        char line[BUFSIZ];
        if (read(mapsfd, line, sizeof(line)) < 0) die("read maps");
        close(mapsfd);

        unsigned long base = strtol(line, NULL, 16);
        entry_point += base;
    }

    munmap(addr, length);
    close(fd);

    printf("process %s, pid %d, entry %lx\n", filename, pid, entry_point);
    return entry_point;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }

    __TRACE();
    pid_t pid = atoi(argv[1]);
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
        die("ptrace(PTRACE_ATTACH)");
    }

    __TRACE();
    // Get the entry point of the target process
    unsigned long entry_point = get_entry_point(pid);
    unsigned long original_code[CODE_SIZE / sizeof(unsigned long)];

    // Read original code at the entry point
    for (int i = 0; i < CODE_SIZE; i += sizeof(unsigned long)) {
        original_code[i / sizeof(unsigned long)] = ptrace(PTRACE_PEEKTEXT, pid, entry_point + i, NULL);
	printf("%lx\n", original_code[i / sizeof(unsigned long)]);
    }

    // Inject new code at the entry point
    for (int i = 0; i < CODE_SIZE; i += sizeof(unsigned long)) {
        ptrace(PTRACE_POKETEXT, pid, entry_point + i, *(unsigned long *)(injected_code + i));
    }

    // Continue the process to let it execute the injected code
    ptrace(PTRACE_CONT, pid, NULL, NULL);

    // Wait for the process to hit the breakpoint
    waitpid(pid, NULL, 0);

    // Restore the original code
    for (int i = 0; i < CODE_SIZE; i += sizeof(unsigned long)) {
        ptrace(PTRACE_POKETEXT, pid, entry_point + i, original_code[i / sizeof(unsigned long)]);
    }

    // Detach from the process
    ptrace(PTRACE_DETACH, pid, NULL, NULL);

    return 0;
}

