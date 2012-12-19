#include <stdio.h>
#include <signal.h>
#include "sched.h"

#define DELAY_FACTOR 29
struct sched_waitq wq1,wq2;

void waste_time(int cycles) {
	int i,j;
	for (i=0; i<cycles; i++) {
		for(j=0;j<1<<DELAY_FACTOR;j++)
			;
	}
}

init_fn()
{
	sched_waitq_init(&wq1);
	sched_waitq_init(&wq2);
	printf("Created init function!\n");
	switch (sched_fork())
	{
	 case -1:
		fprintf(stderr,"Fork failed\n");
		return -1;
	 case 0:
		printf("Parent forked child process child_fn1() with pid %d\n",sched_getpid());
		child_fn1();
		return 0;
	 default:
		parent_fn();
		break;
	}
	printf("The program is over. Goodbye.\n");
	exit(0);
}

child_fn1() {
	sched_nice(5);
	printf("Starting first pass in child_fn1().\n");
	waste_time(1);
	printf("Completed first pass in child_fn1().\n");
	
	kill(getpid(),SIGABRT);
	
	printf("child_fn1() will now sleep until SIGUSR1.\n");
	sched_sleep(&wq1);
	
	printf("child_fn1() woken up by SIGUSR1.\n");
	printf("Starting second pass in child_fn1().\n");
	waste_time(2);
		
	printf("Completed second pass in child_fn1().\n");
	
	kill(getpid(),SIGABRT);

	printf("child_fn1() will now exit with code 22.\n");
	sched_exit(22);
}

parent_fn() {
	int y,p;
	switch(sched_fork())
	{
	 case -1:fprintf(stderr,"Fork failed\n");
		return;
	 case 0:
		printf("Parent forked child process child_fn2() with pid %d\n",sched_getpid());
		child_fn2();
		printf("child_fn2() will now exit with code 11.\n");
		sched_exit(11);
		return;
	 default:
		while ((p=sched_wait(&y)) > 0) {
			printf("parent_fn() recovered child with pid %d and exit code %d\n",p,y);
			printf("parent_fn() is going to do some CPU-bound operations.\n");
			kill(getpid(),SIGUSR2);
			waste_time(5);
			kill(getpid(),SIGABRT);
		}
		printf("parent_fn() has no more child processes to wait for.\n");
		return;
	}
}

child_fn2() {
	sched_nice(20);
	printf("Starting first pass in child_fn2().\n");
	waste_time(1);
	printf("Completed first pass in child_fn2().\n");
	
	kill(getpid(),SIGABRT);
	kill(getpid(),SIGUSR1);
	
	printf("child_fn2() will now sleep until SIGUSR2.\n");
	kill(getpid(),SIGABRT);
	
	sched_sleep(&wq2);
	printf("child_fn2() woken up by SIGUSR2.\n");
	printf("Starting second pass in child_fn2().\n");
	waste_time(5);
	printf("Completed second pass in child_fn1().\n");
	
	kill(getpid(),SIGABRT);
	
}

wakeup_handler(int sig) {
	if (sig==SIGUSR1)
		sched_wakeup(&wq1);
	else
		sched_wakeup(&wq2);
}

main()
{
 struct sigaction sa;
	sa.sa_flags=0;
	sa.sa_handler=wakeup_handler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGUSR1,&sa,NULL);
	sigaction(SIGUSR2,&sa,NULL);
	sigaction(SIGABRT,&sa,NULL);
	sched_init(init_fn);
	fprintf(stderr,"Whoops\n");
}
