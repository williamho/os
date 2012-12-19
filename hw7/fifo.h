/** fifo.h by William Ho */

#ifndef FIFO_H
#define FIFO_H

#include "sem.h"
#include <sys/mman.h>
#define MYFIFO_BUFSIZ 4096

struct fifo {
	int rd_pos;
	int wr_pos; 
	char rd_lock;
	char wr_lock;
	unsigned long arr[MYFIFO_BUFSIZ]; // Circular buffer
	struct sem rd_sem;
	struct sem wr_sem;
};

void fifo_init(struct fifo *f);
void fifo_wr(struct fifo *f, unsigned long d);
unsigned long fifo_rd(struct fifo *f);

#endif
