#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "sched.h"

#define DELAY_FACTOR 28
struct sched_waitq wq1,wq2;

void waste_time(int amt) {
	int i,j;
	for (i=0; i<amt; i++)
		for (j=0; j<1<<DELAY_FACTOR; j++)
			;
}

init_fn() {
	int i, j, stat;
	printf("Created init function: ");
	printf("pid %d / niceval: %d\n",current->pid, current->stat_prio);
	sched_waitq_init(&wq1);
	sched_waitq_init(&wq2);
	
	// Parent process forks 9 CPU-bound processes
	for (i=1; i<10; i++) {
		switch(sched_fork()) {
		case -1: 
			fprintf(stderr,"Fork failed in pid %d\n",current->pid);
			return -1;
			break;
		case 0:
			sched_nice(20+i*2);
			child_fn();
			sched_exit(0);
			break;
		}
	}
	// Parent process waits for a long time before it 
	//	does its own CPU-bound operations
	kill(getpid(),SIGABRT);
	for (;;) {
		for (i=0; i<40; i++)
			sched_sleep(&wq1);
		kill(getpid(),SIGABRT);
		waste_time(10);
		kill(getpid(),SIGABRT);
	}
}

child_fn() {
	// Child process does some computations and when done, sends a SIGUSR1
	int i, j;
	printf("Forked new process: ");
	printf("pid %d / niceval: %d\n",current->pid, current->stat_prio);
	for (;;) {
		waste_time(1);
		kill(getpid(),SIGABRT);
		kill(getpid(),SIGUSR1);
	}
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
	sched_init(init_fn);
	fprintf(stderr,"Whoops\n");
}