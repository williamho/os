#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "sched.h"

#define DELAY_FACTOR 29

void print_info() {
	printf("pid %d / niceval: %d\n", current->pid, 
			current->stat_prio);
}

init_fn() {
	int i, j, stat;
	printf("Created init function: ");
	print_info();
	
	for (i=1; i<10; i++) {
		switch(sched_fork()) {
		case -1: 
			fprintf(stderr,"Fork failed");
			return -1;
			break;
		case 0:
			sched_nice(20+i*2);
			child_fn();
			break;
		}
	}
	for (;;)
		;
}

child_fn() {
	int i, j;
	printf("Forked new process: ");
	print_info();
	for (;;) {
		for (j=0; j<1<<DELAY_FACTOR; j++)
			;
		kill(getpid(),SIGABRT);
	}
}

main()
{
	struct sigaction sa;
	sa.sa_flags=0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGABRT,&sa,NULL);
	sched_init(init_fn);
	fprintf(stderr,"Whoops\n");
}