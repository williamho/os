#include <stdio.h>
#include <signal.h>
#include "sched.h"

#define DELAY_FACTOR 29
struct sched_waitq wq1,wq2;

init_fn()
{
	int x;
	sched_waitq_init(&wq1);
	sched_waitq_init(&wq2);
	fprintf(stderr,"Hooray made it to init_fn, stkaddr %p\n",&x);
	switch (sched_fork())
	{
	 case -1:
		fprintf(stderr,"fork failed\n");
		return -1;
	 case 0:
		fprintf(stderr,"<<in child addr %p>>\n",&x);
		child_fn1();
		fprintf(stderr,"!!BUG!! at %s:%d\n",__FILE__,__LINE__);
		return 0;
	 default:
		fprintf(stderr,"<<in parent addr %p>>\n",&x);
		parent_fn();
		break;
	}
	exit(0);
}

child_fn1()
{
 int y;
	fprintf(stderr,"Start pass1, child_fn1 &y=%p\n",&y);
	for(y=1;y<1<<DELAY_FACTOR;y++)
		;
	fprintf(stderr,"Done pass 1,child_fn1 y=%d\n",y);
	sched_sleep(&wq1);
	fprintf(stderr,"Resuming child_fn1\n");
	for(y=1;y<1<<DELAY_FACTOR;y++)
		;
	fprintf(stderr,"Done pass 2,child_fn1 y=%d\n",y);
	
	kill(getpid(),SIGUSR2);
	
	sched_exit(22);
}

parent_fn()
{
 int y,p;
	fprintf(stderr,"Wow, made it to parent, stkaddr=%p\n",&y);
	switch(sched_fork())
	{
	 case -1:fprintf(stderr,"Fork failed\n");
		return;
	 case 0:
		child_fn2();
		sched_exit(11);
		fprintf(stderr,"!!BUG!! at %s:%d\n",__FILE__,__LINE__);
		return;
	 default:
		while ((p=sched_wait(&y))>0)
			fprintf(stderr,"Child pid %d return code %d\n",p,y);
		return;
	}
}

child_fn2()
{
 int y;
	sched_nice(4);
	fprintf(stderr,"Start pass1, child_fn2 &y=%p\n",&y);
	for(y=0;y<1<<DELAY_FACTOR;y++)
		;
	fprintf(stderr,"Done pass 1,child_fn2 y=%d\n",y);
	sched_sleep(&wq2);
	fprintf(stderr,"Resuming child_fn2\n");
	for(y=0;y<1<<DELAY_FACTOR;y++)
		;
	fprintf(stderr,"Done pass 2,child_fn2 y=%d\n",y);
	
}

wakeup_handler(int sig)
{
	if (sig==SIGUSR1)
		sched_wakeup(&wq1);
	else
		sched_wakeup(&wq2);
}

abrt_handler(int sig)
{
	sched_ps();
}

main()
{
 struct sigaction sa;
	fprintf(stderr,"Starting\n");
	sa.sa_flags=0;
	sa.sa_handler=wakeup_handler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGUSR1,&sa,NULL);
	sigaction(SIGUSR2,&sa,NULL);
	sa.sa_handler=abrt_handler;
	sigaction(SIGABRT,&sa,NULL);
	sched_init(init_fn);
	fprintf(stderr,"Whoops\n");
}
