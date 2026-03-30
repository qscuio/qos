#include <stdio.h>
#include <unistd.h>

int hello()
{
    return 0;
}

int hello1()
{
    printf("hello world");
    return 0;
}

int main() {
    while (1) {
	hello();
        sleep(1);
    }
    return 0;
}

