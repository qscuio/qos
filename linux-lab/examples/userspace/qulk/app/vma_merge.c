#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <linux/userfaultfd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

int main(void)
{
    int pid = getpid();
    char cmd[1024] = {0,};
    int r;
    sprintf(cmd, "cat /proc/%d/maps", pid);

    // Case-1: Can it be merged by vma_merge? 
    void *p1 = mmap((void *)0x24000, 0x1000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    void *p2 = mmap((void *)0x26000, 0x1000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    system(cmd);
    
    // Case-2: mprotect((void *)) on those areas?
    r = mprotect((void *)0x24000, 0x3000, PROT_READ);
    if (r) {
        printf("\n\nmprotet error : %d, %s\n", errno, strerror(errno));  // error!!
    }

    // Case-3: merged at the time of mmap
    munmap(p1, 0x1000);
    munmap(p2, 0x1000); 
    p1 = mmap((void *)0x24000, 0x1000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    p2 = mmap((void *)0x25000, 0x1000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    printf("\n\n");
    system(cmd);

    // Case-4: can't be merged since they have different prot
    munmap(p1, 0x1000);
    munmap(p2, 0x1000); 
    p1 = mmap((void *)0x24000, 0x1000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    p2 = mmap((void *)0x25000, 0x1000, PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    printf("\n\n");
    system(cmd);

    // Case-5: merged explicitly by mprotect
    munmap(p1, 0x1000);
    munmap(p2, 0x1000);
    p1 = mmap((void *)0x24000, 0x1000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    p2 = mmap((void *)0x25000, 0x1000, PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    mprotect((void *)0x24000, 0x2000, PROT_READ | PROT_WRITE);
    printf("\n\n");
    system(cmd);
    
    return 0;
}
