#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h> 
#include <string.h>

void signal_handler(int signo, siginfo_t *info, void *context)
{
	if (signo == SIGINT)
	{
		printf("Exited on signal interrupt\n");
		exit(1);
	}

	if (signo == SIGSEGV)
	{
		printf("Exited on signal segement fault\n");
		exit(1);
	}

	printf("recevied signal %d, %s\n", signo, strsignal(signo));
}

void signal_init(void)
{
	int i = 0;
	struct sigaction act;
	act.sa_sigaction = signal_handler;
	act.sa_flags = SA_SIGINFO;
	sigemptyset(&act.sa_mask);

	for (int i = 1; i < NSIG; ++i) {
		if (1 || (i != SIGKILL && i != SIGSTOP)) {
			if (sigaction(i, &act, NULL) == -1) {
				fprintf(stderr, "Cannot handle signal %d\n", i);
			}
		}
	}
}

static void *check = NULL;
void ____check_malloc(void)
{
	check = malloc(112);
}

void __check_malloc(void)
{
	____check_malloc();
}

void check_malloc(void)
{
	__check_malloc();
}

void ____check_free(void)
{
	free(check);
}

void __check_free(void)
{
	____check_free();
}

void check_free(void)
{
	__check_free();
}

void check_malloc_free(void)
{
	char *pt = NULL;

	pt = malloc(10);
	free(pt);
	pt = malloc(11);
	free(pt);
	pt = malloc(12);
	free(pt);

	check_malloc();
	check_free();
}

struct sfoo {
	int x;
	int y;
};

int foo (int a, int b)
{
	return a + b;
}

int bar (int a, int b)
{
	return foo(a , b);
}

int foo_sfoo(struct sfoo sf) {
	return bar(sf.x, sf.y);
}

int foo_psfoo(struct sfoo *psf) {
	return bar(psf->x, psf->y);
}

int main() {
	struct sfoo sf = {.x = 10, .y = 20 };
	pid_t pid = getpid();
	char command[256] = {0};
	sprintf(command, "cat /proc/%d/maps", pid);
	signal_init();
	while (1)
	{
		printf("Hello world\n");
		printf("Hi %d\n", getpid());
		foo(1, 2);
		bar(3, 4);
		foo_sfoo(sf);
		foo_psfoo(&sf);
		check_malloc_free();
		system(command);
		sleep(2);
	}

	return 0;

}
