// target.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main() {
    char buffer[100];

    // Fill the buffer with a known pattern
    for (int i = 0; i < sizeof(buffer); i++) {
        buffer[i] = (char)(i % 256);
    }

    printf("Target process PID: %d\n", getpid());
    printf("Buffer address: %p\n", (void*)buffer);

    // Keep the process running
    while (1) {
        sleep(1);
    }

    return 0;
}

