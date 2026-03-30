#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

extern const char *__progname;

int main(void) {
    char exe[1024];
    int ret;

    ret = readlink("/proc/self/exe",exe,sizeof(exe)-1);
    if(ret ==-1) {
        fprintf(stderr,"ERRORRRRR\n");
        exit(1);
    }
    exe[ret] = 0;
    printf("I am %s\n",exe);
    printf("I am %s\n",__progname);
}
