#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    // Size of the memory region
    size_t size = 4096;

    // Allocate a region of memory using mmap
    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // x86-64 machine code for a function that adds two integers
    // int add(int a, int b) { return a + b; }
    unsigned char code[] = {
        0x55,                   // push rbp
        0x48, 0x89, 0xe5,       // mov rbp, rsp
        0x89, 0x7d, 0xfc,       // mov [rbp-4], edi (store a)
        0x89, 0x75, 0xf8,       // mov [rbp-8], esi (store b)
        0x8b, 0x55, 0xfc,       // mov edx, [rbp-4] (load a)
        0x8b, 0x45, 0xf8,       // mov eax, [rbp-8] (load b)
        0x01, 0xd0,             // add eax, edx (a + b)
        0x5d,                   // pop rbp
        0xc3                    // ret
    };

    // Copy the machine code into the allocated memory
    memcpy(mem, code, sizeof(code));

    // Change memory protection to allow execution
    if (mprotect(mem, size, PROT_READ | PROT_EXEC) == -1) {
        perror("mprotect");
        exit(1);
    }

    // Cast the memory region to a function pointer
    int (*add_func)(int, int) = (int (*)(int, int))mem;

    // Call the function and print the result
    int result = add_func(3, 5);
    printf("Result: %d\n", result); // Expected output: 8

    // Clean up
    if (munmap(mem, size) == -1) {
        perror("munmap");
        exit(1);
    }

    return 0;
}

