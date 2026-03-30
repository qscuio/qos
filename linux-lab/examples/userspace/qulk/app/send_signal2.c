#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void signal_handler(int signo, siginfo_t *info, void *context)
{
    printf("Received signal %d with data: %d\n", signo, info->si_int);
}

int main()
{
    struct sigaction act;
    act.sa_sigaction = signal_handler;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);

    if (sigaction(SIGUSR1, &act, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    printf("Process PID: %d\n", getpid());

    // Loop indefinitely
    while (1) {
        sleep(1);
    }

    return 0;
}

