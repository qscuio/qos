#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
#include<unistd.h>
#include <sys/mman.h>
#include <malloc.h>

void *threadfn1(void *p1)
{
	int num;
	int val=90;
	int *p=&val;
	printf("val addr=%p\n",&val);
	for(num=0;num<0x500;num++){
	    *p=20;
	    p++;
	}
	while(1){
		puts("thread1");
		sleep(10);
	}
	return NULL;
}

void *threadfn2(void *p)
{
    while(1){
	puts("thread2");
	sleep(10);
    }
    return NULL;
}

int main()
{
    void *p1,*p2;
    pthread_t t1,t2,t3;
    int page_size=sysconf(_SC_PAGESIZE);
    printf("page size=%x\n",page_size);
    pthread_attr_t attr;

    struct sched_param param;

    pthread_attr_init(&attr);
    p1=mmap(0,0x20000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);

    printf("mmap ret=%p\n",p1); // the return address is page aligned

    mprotect(p1+page_size,page_size,PROT_NONE);
    mprotect(p1+0x1f000,0x1000,PROT_NONE);

    p2=malloc(0x20000);
    printf("malloc ret=%p\n",p2); // malloc return address is not page aligned

    pthread_attr_setstack(&attr,p1+0x1000,0x1d000);

    pthread_create(&t1,&attr,threadfn1,NULL);

    pthread_attr_setstack(&attr,p2,0x20000);

    pthread_create(&t2,&attr,threadfn2,NULL);
    sleep(100);
    puts("end test");
    return 0;
}
