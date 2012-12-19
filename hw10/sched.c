#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/time.h>
#include "sched.h"

void *create_stack();

// Globals
int need_resched;
sigset_t mask, usrmask;
unsigned long ticks;
struct sched_proc **procs;

struct sched_waitq child_wq;
struct sched_waitq *run_q_active, *run_q_expired;
struct itimerval timer;

struct sigaction tickaction, abrtaction;

void sched_init(void (*init_fn)()) {
	ticks = 0;
	procs = malloc(SCHED_NPROC*sizeof(struct sched_proc *));
	
	// Set up signal masks
	sigemptyset(&mask);
	sigaddset(&mask, SIGVTALRM);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	
	sigemptyset(&usrmask);
	sigaddset(&usrmask, SIGUSR1);
	sigaddset(&usrmask, SIGUSR2);

	// Initialize wait queues and run queues
	sched_waitq_init(&child_wq);
	run_q_active = malloc(sizeof(struct sched_waitq));
	run_q_expired = malloc(sizeof(struct sched_waitq));
	sched_waitq_init(run_q_active);
	sched_waitq_init(run_q_expired);
	
	// Set up periodic timer
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = TIMER_USECS;
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = TIMER_USECS;
	setitimer(ITIMER_VIRTUAL, &timer, 0);
	
	// Interrupt handlers
	tickaction.sa_handler = sched_tick;
	sigemptyset(&tickaction.sa_mask);
	tickaction.sa_flags = 0;
	sigaction(SIGVTALRM,&tickaction,NULL);

	abrtaction.sa_handler = sched_ps;
	sigemptyset(&abrtaction.sa_mask);
	abrtaction.sa_flags = 0;
	sigaction(SIGABRT,&abrtaction,NULL);
	
	// Start init process (pid = 1)
	procs[1] = malloc(sizeof(struct sched_proc));
	procs[1]->stackptr = create_stack();
	procs[1]->pid = 1;
	procs[1]->state = SCHED_RUNNING;
	procs[1]->quantum = DYN_PRIO_DFL;
	procs[1]->stat_prio = STAT_PRIO_DFL;
	current = procs[1];
	
	struct savectx ctx;
	savectx(&ctx);
	ctx.regs[JB_PC] = init_fn;
	ctx.regs[JB_BP] = procs[1]->stackptr;
	ctx.regs[JB_SP] = procs[1]->stackptr;
	restorectx(&ctx,0);
}

int sched_fork() {
	int cpid = 0;
	long adj;
	
	block_sigs();
	
	// Loop through process numbers to find an unused one
	for (cpid=1; cpid<SCHED_NPROC; cpid++)
		if (procs[cpid] == NULL)
			break;
	
	if (cpid == SCHED_NPROC) {
		fprintf(stderr,"Exceeded max number of processes\n");
		return -1;
	}
	
	procs[cpid] = malloc(sizeof(struct sched_proc));
	
	// Initialize the child process
	procs[cpid]->pid = cpid;
	procs[cpid]->ppid = current->pid;
	procs[cpid]->state = SCHED_READY;
	procs[cpid]->stat_prio = STAT_PRIO_DFL;
	procs[cpid]->quantum = current->quantum/2;
	procs[cpid]->cputime = 0;
	procs[cpid]->numchildren = 0;
	current->numchildren++;
	
	// Put child process on run queue
	put_on_run_q(procs[cpid],run_q_active);
	
	// Initialize child's stack and copy contents of parent's stack
	if ((procs[cpid]->stackptr = create_stack()) == NULL)
		return -1;

	// Modify registers
	adj = ((char*)procs[cpid]->stackptr)-((char*)current->stackptr);
	memcpy(procs[cpid]->stackptr-STACK_SIZE,current->stackptr-STACK_SIZE,STACK_SIZE);
	adjstack(procs[cpid]->stackptr-STACK_SIZE,procs[cpid]->stackptr,adj);

	*(&cpid + adj/(sizeof(int))) = 0; // Set return value of fork() to 0 in child's stack
	
	// Save context of child, adjust base/stack pointers to child's stack
	if (savectx(&procs[cpid]->ctx) == 0) {
		procs[cpid]->ctx.regs[JB_BP] += adj;
		procs[cpid]->ctx.regs[JB_SP] += adj;
	}

	unblock_sigs();
	return cpid;
}

void sched_exit(int code) {	
	block_sigs();
	
	put_on_run_q(current,NULL);
	current->state = SCHED_ZOMBIE;
	current->exit_code = code;
	current->quantum = 0;
	
	// If this process has child processes, set their ppid to 1 (init process)
	int i;
	for (i=1; i<SCHED_NPROC; i++)
		if (procs[i] != NULL && procs[i]->ppid == current->pid)
			procs[i]->ppid = 1;
	
	// Check if parent sleeping
	if (procs[current->ppid]->state == SCHED_SLEEPING)
		sched_wakeup(&child_wq);
	
	unblock_sigs();
	sched_switch();
}

int sched_wait(int *exit_code) {
	if (current->numchildren == 0)
		return -1;

	int cpid;
	for (;;) {
		block_sigs();
		for (cpid=1; cpid<SCHED_NPROC; cpid++) {
			// Found a zombie child process
			if (procs[cpid] != NULL && procs[cpid]->ppid == current->pid 
					&& procs[cpid]->state == SCHED_ZOMBIE) {
				*exit_code = procs[cpid]->exit_code;				
				
				// Free its resources
				munmap(procs[cpid]->stackptr-STACK_SIZE,STACK_SIZE);
				free(procs[cpid]);
				procs[cpid] = NULL;
				current->numchildren--;
				
				//sched_ps();
				
				unblock_sigs();
				return cpid;
			}
		}
		
		unblock_sigs();
		if (cpid == SCHED_NPROC) // No zombie children. Back to sleep.
			sched_sleep(&child_wq);
	}
}

void sched_sleep(struct sched_waitq *waitq) {
	block_sigs();
	put_on_run_q(current,NULL); // Remove from run queues
	current->state = SCHED_SLEEPING;

	waitq->procs[current->pid] = current; // Put task on waitq
	waitq->numprocs++;
	current->wq = waitq;
	unblock_sigs();

	sched_switch();
}

void sched_wakeup(struct sched_waitq *waitq) {
	int pid;
	block_sigs();
	for (pid=1; pid<SCHED_NPROC; pid++) {
		if (waitq->procs[pid] != NULL) {
			procs[pid]->state = SCHED_READY;
			if (procs[pid]->quantum > current->quantum) // Check dynamic priority
				need_resched = 1;
			put_on_run_q(waitq->procs[pid],run_q_active);
			waitq->procs[pid] = NULL; // Remove from waitq
			procs[pid]->wq = NULL;
			waitq->numprocs--;
		}
	}
	unblock_sigs();
	
	if (need_resched) {
		need_resched = 0;
		sched_switch();
	}
}

void put_on_run_q(struct sched_proc *proc, struct sched_waitq *new_rq) {
	// Add process to run queue (or if new_rq is NULL, remove from run queue)
	proc->state = SCHED_READY;
	block_sigs();
	
	if (proc->rq != NULL) {
		proc->rq->procs[proc->pid] = NULL;
		proc->rq->numprocs--;
	}
	if (new_rq != NULL) {
		new_rq->procs[proc->pid] = proc;
		new_rq->numprocs++;
	}	
	
	proc->rq = new_rq;
	unblock_sigs();
}

void sched_nice(int niceval) {
	if (niceval > STAT_PRIO_MAX)
		niceval = STAT_PRIO_MAX;
	else if (niceval < STAT_PRIO_MIN)
		niceval = STAT_PRIO_MIN;
	
	current->stat_prio = niceval;
}

int sched_getpid() {
	return current->pid;
}

int sched_getppid() {
	return current->ppid;
}

unsigned long sched_gettick() {
	return ticks;
}

void sched_ps() {
	int i;
	printf("PID PPID STATE   WAITQ.ADDRESS    BASE.ADDRESS S.PRIO D.PRIO TIME\n");

	for (i=1; i<SCHED_NPROC; i++) {
		if (procs[i] != NULL) {
			printf("%3d %4d ",i,procs[i]->ppid);
			switch(procs[i]->state) {
			case SCHED_READY:
				printf("READY               -  ");
				break;
			case SCHED_RUNNING:
				printf("RUNNING             -  ");
				break;
			case SCHED_SLEEPING:
				printf("SLEEP   %13p  ",procs[i]->wq);
				break;
			case SCHED_ZOMBIE:
				printf("ZOMBIE              -  ");
				break;
			}
			
			printf("%p ",procs[i]->stackptr);
			printf("%6d ",procs[i]->stat_prio);
			printf("%6d ",procs[i]->quantum); 
			printf("%4lu\n",procs[i]->cputime);
		}
	}
}

void sched_switch() {
	int next, i, best_i, best_val;
	
	// If no runnable tasks
	if (run_q_active->numprocs == 0 && run_q_expired->numprocs == 0) {
		printf("No runnable tasks. Sleeping.\n");
		sigsuspend(&usrmask);
	}
	
	// If no tasks on active queue, the expired queue becomes the active queue
	if (run_q_active->numprocs == 0 && run_q_expired->numprocs > 0) {
		struct sched_waitq *tmp;
		tmp = run_q_active;
		run_q_active = run_q_expired;
		run_q_expired = tmp;
		
		for (i=1; i<SCHED_NPROC; i++) {
			if (procs[i] != NULL && procs[i]->state != SCHED_ZOMBIE) {
				procs[i]->quantum += STAT_PRIO_MAX - procs[i]->stat_prio + 1;
				if (procs[i]->quantum > DYN_PRIO_MAX)
					procs[i]->quantum = DYN_PRIO_MAX;
			}
		}
		
	}
	
	// Choose the ready process with the highest priority (quantum)
	best_i = best_val = -1;
	for (i=1; i<SCHED_NPROC; i++) {
		if (run_q_active->procs[i] != NULL && run_q_active->procs[i]->quantum > best_val) {
			best_val = run_q_active->procs[i]->quantum;
			best_i = i;
		}
	}

	unblock_sigs();
	
	// If the best process is not the current one, restore its context
	if (best_i > 0 && savectx(&current->ctx) == 0) {
		if (current->rq != NULL)
			current->state = SCHED_READY;
		current = procs[best_i];
		restorectx(&current->ctx,1);
	}
	current->state = SCHED_RUNNING;
}

void sched_tick() {
	int i,j,flag;
	
	current->quantum--;
	current->cputime++;
	ticks++;
	
	block_sigs();
	
	// See if current process' time is up
	if (current->quantum <= 0) {
		put_on_run_q(current,run_q_expired);
		need_resched = 1;
	}

	unblock_sigs();
	if (need_resched) {
		need_resched = 0;
		sched_switch();
	}
	
}

void sched_waitq_init(struct sched_waitq *wq) {
	wq->procs = malloc(SCHED_NPROC*sizeof(struct sched_proc *));
	wq->numprocs = 0;
}

void block_sigs() {
	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
		perror("Could not block SIGVTALRM, SIGUSER1, SIGUSR2");
}

void unblock_sigs() {
	if (sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0)
		perror("Could not unblock SIGVTALRM, SIGUSER1, SIGUSR2");
}

void *create_stack() {
	void *newsp;
	if ((newsp=mmap(0,STACK_SIZE,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,0,0))==MAP_FAILED)
		return NULL;
	return newsp+STACK_SIZE;
}
