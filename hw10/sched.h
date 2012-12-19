#ifndef SCHED_H
#define SCHED_H

#include "savectx64.h"
#include <sys/time.h>

#define SCHED_NPROC		256
#define SCHED_READY 	0
#define SCHED_RUNNING 	1
#define SCHED_SLEEPING 	2
#define SCHED_ZOMBIE 	3

#define TIMER_USECS 50000

#define STAT_PRIO_MIN 0
#define STAT_PRIO_DFL 20
#define STAT_PRIO_MAX 39
#define DYN_PRIO_DFL 20
#define DYN_PRIO_MAX 100

#define STACK_SIZE 65536

struct sched_proc *current;

struct sched_proc {
	int pid;
	int ppid;
	int state;
	
	int stat_prio;
	
	struct sched_waitq *wq;
	struct sched_waitq *rq;
	long quantum;
	unsigned long cputime;
	
	int numchildren;
	
	struct savectx ctx;
	void *stackptr;
	int exit_code;
};

struct sched_waitq {
	struct sched_proc **procs;
	int numprocs;
};

// Functions
void block_sigs();
void unblock_sigs();

int savectx(struct savectx *ctx);
void restorectx(struct savectx *ctx,int retval);
int adjstack(void *lim0,void *lim1,unsigned long adj);

void sched_init(void (*init_fn)());
int sched_fork();
void sched_exit(int code);
int sched_wait(int *exit_code);
void sched_sleep(struct sched_waitq *waitq);
void sched_wakeup(struct sched_waitq *waitq);
void sched_nice(int niceval);
int sched_getpid();
int sched_getppid();
void sched_ps();
void sched_switch();
void sched_tick();
unsigned long sched_gettick();
void sched_waitq_init(struct sched_waitq *wq);
void put_on_run_q(struct sched_proc *proc, struct sched_waitq *rq);

#endif