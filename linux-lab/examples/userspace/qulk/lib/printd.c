#include <dirent.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/syslog.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void printd(const char *fmt, ...) {
    int     rc, fd;
    va_list args;
    char    line[ 1024 ];

    va_start(args, fmt);
    rc = vsprintf(line, fmt, args);
    va_end(args);

    fd = open("/dev/console", O_RDWR, 0);
    if (fd > 0) {
        write(fd, line, rc);
        close(fd);
    }
}
