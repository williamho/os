/** sync.c by William Ho. 
	Tests the FIFO implemented in fifo.c by having multiple writers write to
	the same FIFO (via mmap's MAP_SHARED) and having only one reader.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "sem.h"
#include "fifo.h"

#define SEQSHIFT 24
#define GETSENDER(x) ((x)>>SEQSHIFT)
#define GETSEQNUM(x) ((x)&((1<<SEQSHIFT)-1))

void test(int total_procs, int num_data);
void sigchld_h(int signum);

struct fifo *f;
main(int argc, char *argv[]) {
	char *endptr;
	int num_procs; 
	int num_data;
	
	f = mmap(0, sizeof(struct fifo), PROT_READ|PROT_WRITE, 
						MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	fifo_init(f);
	
	my_procnum = 0;
	
	if (argc > 2) {
		num_procs = strtol(argv[1],&endptr,10);
		if (*endptr != '\0') {
			fprintf(stderr,"Argument 1: Invalid number of writers: %s\n",argv[1]);
			exit(-1);
		}
		if (num_procs < 1 || num_procs > N_PROC) {
			fprintf(stderr,"Argument 1: Number of writers must be between 1 and %d.\n",N_PROC);
			exit(-1);
		}
		
		num_data = strtol(argv[2],&endptr,10);
		if (*endptr != '\0') {
			fprintf(stderr,"Argument 2: Invalid number of data: %s\n",argv[2]);
			exit(-1);
		}
		if (num_data < 1 || num_data > (1<<SEQSHIFT)-1) {
			fprintf(stderr,"Argument 2: Number of data must be between 1 and %d",(1<<SEQSHIFT)-1);
			exit(-1);
		}
		
		printf("Testing FIFO with %d writers each sending %d unsigned longs.\n",num_procs,num_data);
		test(num_procs,num_data);
	}
	else
		fprintf(stderr,"Usage: %s num_writers num_data\n",argv[0]);
}

void test(int total_procs, int num_data) {
	unsigned long seq, start, rcv, expected, i;
	unsigned long latest[N_PROC];
	int sender, procs_done;
	
	for (my_procnum=0; my_procnum<total_procs; my_procnum++) {
		switch(fork()) {
		case -1:
			perror("fork");
			exit(-1);
			break;
		case 0:
			latest[my_procnum] = -1;
			break;
		default:
			start = my_procnum<<SEQSHIFT;
			for (seq=start; seq<start+num_data; seq++)
				fifo_wr(f,seq);
			exit(0);
		}
	}
	
	my_procnum = 0; // Readers have separate my_procnum space: rd_sem used for reading
	procs_done = 0;
	for (i=0; procs_done!=total_procs; i++) {
		rcv = fifo_rd(f);
		sender = (int) GETSENDER(rcv);
		expected = latest[sender]+1;
		seq = GETSEQNUM(rcv);
		
		if (GETSEQNUM(rcv) != latest[sender]+1) {
			fprintf(stderr,"Error (read #%lu): Reader received seq %lu from proc #%d (expected %lu).\n",
						i,seq,sender,expected);
			exit(-1);
		}
		latest[sender] = seq;
		
		if (seq == 0)
			printf("Received first datum from proc #%d after %lu calls to fifo_rd().\n",sender,i+1);
		if (seq == num_data-1) {
			procs_done++;
			printf("All data from proc #%d received after %lu calls to fifo_rd().\nStill waiting on %d proccesses.\n",sender,i+1,total_procs-procs_done);
		}
	}
	printf("Received all data in order!\n");
	exit(0);
}
