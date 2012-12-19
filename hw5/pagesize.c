/* pagesize.c by William Ho
	Determine the size of a page experimentally by handling segmentation faults. 
*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void sigsegv_handler(int signum);
char *cptr, *fault1, *fault2;

main() {
	char tmp;
	fault1 = fault2 = NULL;
	signal(SIGSEGV,sigsegv_handler);
	
	if ((cptr = sbrk(0)) < 0) {
		perror("sbrk wirh argument 0");
		exit(-1);
	}
	while(1)
		tmp = *cptr++;
}

void sigsegv_handler(int signum) {
	if (fault1 == NULL) {
		fault1 = cptr;
		printf("First faulting address occurred at %p.\n",fault1);
	}
	else {
		fault2 = cptr;
		printf("Second faulting address occurred at %p.\n",fault2);
		printf("Page size determined experimentally to be %ld bytes.\n",fault2-fault1);
		exit(0);
	}
	
	if ((cptr = sbrk(1)) < 0) {
		perror("sbrk wirh argument 1");
		exit(-1);
	}
}