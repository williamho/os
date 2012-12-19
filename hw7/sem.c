/** sem.c by William Ho */

#include "sem.h"
#include <string.h>
#include <unistd.h>

void sigusr1_h() { return; }

int mutex_trylock(volatile char *lock) { return !tas(lock); }

void mutex_lock(volatile char *lock) { while (tas(lock)); }

void mutex_unlock(volatile char *lock) { *lock = 0; }

void sem_init(struct sem *s, int count) {
	s->available = count;
	s->lock = 0;
	
	memset(s->waiting, 0, sizeof(s->waiting));
	memset(s->pids, 0, sizeof(s->pids));
	
	sigfillset(&s->sigmask);
	sigdelset(&s->sigmask,SIGINT);
	sigdelset(&s->sigmask,SIGUSR1);
	signal(SIGUSR1,sigusr1_h);
}

int sem_try(struct sem *s) {
	mutex_lock(&s->lock);
	if (s->available > 0) {
		s->available--;
		mutex_unlock(&s->lock);
		return 1;
	}
	else {
		mutex_unlock(&s->lock);
		return 0;
	}
}

void sem_wait(struct sem *s) {	
	s->waiting[my_procnum] = 1;
	s->pids[my_procnum] = getpid();
	
	while (1) {
		mutex_lock(&s->lock);
		if (s->available > 0) {
			s->available--;
			s->waiting[my_procnum] = 0;
			mutex_unlock(&s->lock);
			return;
		}
		else {
			mutex_unlock(&s->lock);
			sigsuspend(&s->sigmask);
		}
	}
}
void sem_inc(struct sem *s) {
	int i;
	mutex_lock(&s->lock);
	s->available++;
	
	// Wake the waiting processes
	for (i=0; i<N_PROC; i++) {
		if (s->waiting[i]) {
			kill(s->pids[i],SIGUSR1);
		}
	}
	
	mutex_unlock(&s->lock);
}
