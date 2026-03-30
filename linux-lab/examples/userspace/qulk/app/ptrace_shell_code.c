#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/user.h>
#include <unistd.h>
#include <errno.h>

#define SHELLCODE_SIZE 44
//
//Explanation
//* Attach to the target process: ptrace(PTRACE_ATTACH, pid, NULL, NULL).
//* Wait for the target process to stop: waitpid(pid, NULL, 0).
//* Save the original code: Use ptrace(PTRACE_PEEKTEXT, pid, (void *)(regs.rip + i), NULL) to read the original code at the injection point.
//* Inject the shellcode: Use ptrace(PTRACE_POKETEXT, pid, (void *)(regs.rip + i), *(long *)(sc_ptr + i)) to write the shellcode into the target process's memory.
//* Adjust the instruction pointer: Modify regs.rip to point to the start of the shellcode.
//* Continue execution: ptrace(PTRACE_CONT, pid, NULL, NULL).
//* Wait for the target process to stop: waitpid(pid, NULL, 0) after executing the shellcode.
//* Restore the original code: Use ptrace(PTRACE_POKETEXT, pid, (void *)(original_regs.rip + i), original_code[i / sizeof(long)]) to write the original code back.
//* Restore the original register values: Use ptrace(PTRACE_SETREGS, pid, NULL, &original_regs) to set the original register values.
//* Detach: ptrace(PTRACE_DETACH, pid, NULL, NULL).

// Define a simple shellcode that calls write to print "Hello, World!" to stdout
unsigned char shellcode[SHELLCODE_SIZE] = {
    0xeb, 0x1a,                         // jmp    <string>
    0x48, 0x31, 0xc0,                   // xor    %rax,%rax
    0x48, 0x89, 0xc2,                   // mov    %rax,%rdx
    0x48, 0x89, 0xc6,                   // mov    %rax,%rsi
    0x48, 0x8d, 0x3d, 0x0d, 0x00, 0x00, 0x00, // lea    0xd(%rip),%rdi        # <string>
    0xb0, 0x01,                         // mov    $0x1,%al
    0x0f, 0x05,                         // syscall
    0xeb, 0xfe,                         // jmp    <self>
    '+', '+', '+', '+', '+', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!', '\n' , 0// <string>
};

void inject_code(pid_t pid) {
    struct user_regs_struct regs, original_regs;
    unsigned long original_code[SHELLCODE_SIZE];
    unsigned char *sc_ptr = shellcode;
    int i;

    // Attach to the target process
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
        perror("ptrace attach");
        exit(EXIT_FAILURE);
    }

    // Wait for the target process to stop
    waitpid(pid, NULL, 0);

    // Get the current register values
    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace getregs");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        exit(EXIT_FAILURE);
    }
    original_regs = regs;

    // Save the original code
    for (i = 0; i < SHELLCODE_SIZE; i += sizeof(long)) {
        original_code[i / sizeof(long)] = ptrace(PTRACE_PEEKTEXT, pid, (void *)(regs.rip + i), NULL);
    }

    // Inject the shellcode into the target process
    for (i = 0; i < SHELLCODE_SIZE; i += sizeof(long)) {
        if (ptrace(PTRACE_POKETEXT, pid, (void *)(regs.rip + i), *(long *)(sc_ptr + i)) == -1) {
            perror("ptrace poketext");
            ptrace(PTRACE_DETACH, pid, NULL, NULL);
            exit(EXIT_FAILURE);
        }
    }

    // Set the instruction pointer to the start of the shellcode
    regs.rip += 2; // Adjust RIP to point to the start of the shellcode
    if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace setregs");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        exit(EXIT_FAILURE);
    }

    // Continue the execution of the target process
    if (ptrace(PTRACE_CONT, pid, NULL, NULL) == -1) {
        perror("ptrace cont");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        exit(EXIT_FAILURE);
    }

    // Wait for the target process to stop (after executing shellcode)
    waitpid(pid, NULL, 0);

    // Restore the original code
    for (i = 0; i < SHELLCODE_SIZE; i += sizeof(long)) {
        if (ptrace(PTRACE_POKETEXT, pid, (void *)(original_regs.rip + i), original_code[i / sizeof(long)]) == -1) {
            perror("ptrace restore poketext");
            ptrace(PTRACE_DETACH, pid, NULL, NULL);
            exit(EXIT_FAILURE);
        }
    }

    // Restore the original register values
    if (ptrace(PTRACE_SETREGS, pid, NULL, &original_regs) == -1) {
        perror("ptrace restore setregs");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        exit(EXIT_FAILURE);
    }

    // Detach from the target process
    if (ptrace(PTRACE_DETACH, pid, NULL, NULL) == -1) {
        perror("ptrace detach");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t target_pid = atoi(argv[1]);
    inject_code(target_pid);

    return 0;
}

