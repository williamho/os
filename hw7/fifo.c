/** fifo.c by William Ho */

#include "fifo.h"

void fifo_init(struct fifo *f) {
	f->rd_pos = f->wr_pos = 0;
	f->rd_lock = f->wr_lock = 0;
	sem_init(&f->rd_sem,0);
	sem_init(&f->wr_sem,MYFIFO_BUFSIZ);
}

void fifo_wr(struct fifo *f, unsigned long d) {
	while(1) {
		sem_wait(&f->wr_sem);
		
		if (mutex_trylock(&f->wr_lock)) {
			f->arr[f->wr_pos] = d;
			f->wr_pos = (f->wr_pos + 1)%MYFIFO_BUFSIZ;
			mutex_unlock(&f->wr_lock);
			sem_inc(&f->rd_sem);
			return;
		}
		else
			sem_inc(&f->wr_sem);
	}
}

unsigned long fifo_rd(struct fifo *f) {
	unsigned long retval;
	while(1) {
		sem_wait(&f->rd_sem);
		
		if (mutex_trylock(&f->rd_lock)) {
			retval = f->arr[f->rd_pos];
			f->rd_pos = (f->rd_pos + 1)%MYFIFO_BUFSIZ;
			mutex_unlock(&f->rd_lock);
			sem_inc(&f->wr_sem);
			return retval;
		}
		else
			sem_inc(&f->rd_sem);
	}
}
