#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
    void *p1 = NULL;
    void *p2 = NULL;
    void *p3 = NULL;

    daemon(0, 0);

    p1 = mmap((void *)0x24000, 0x1000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    p2 = mmap((void *)0x240000, 0x4000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    p3 = mmap((void *)0x2400000, 0x8000, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    pause();
    munmap(p1, 0x1000);
    munmap(p2, 0x1000);
    munmap(p3, 0x1000);
    return 0;
}
