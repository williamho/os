/** sem.h by William Ho */
#ifndef SEM_H
#define SEM_H

#include <sys/types.h>
#include <signal.h>
#define N_PROC 64

int my_procnum;
struct sem {
	char lock;
	int available;
	int pids[N_PROC];
	char waiting[N_PROC];
	sigset_t sigmask;
};

void sigusr1_h();

int tas(volatile char *lock);
int mutex_trylock(volatile char *lock);
void mutex_lock(volatile char *lock);
void mutex_unlock(volatile char *lock);

void sem_init(struct sem *s, int count);
int sem_try(struct sem *s);
void sem_wait(struct sem *s);
void sem_inc(struct sem *s);

#endif
