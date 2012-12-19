#include <stdio.h>
#include <time.h>
#include <sys/syscall.h>

void emptyfn() {}

main() {
	unsigned long i, numticks, numloops;
	clock_t startclock, endclock;
	double secs, nanosecs;
	
	// Empty loop test
	numloops = 25000000000;
	startclock = clock();
	for (i=0; i<numloops; i++);

	numticks = clock()-startclock;
	secs = (double)numticks/CLOCKS_PER_SEC;
	nanosecs = secs*1E9;
	
	printf("A. Empty loop:\n");
	printf("\t%ld clock ticks (%lf secs) for %ld iterations.\n", numticks, secs, numloops);
	printf("\t%lf clock ticks (%lf nanosecs) for 1 iteration.\n\n", (double)numticks/numloops, nanosecs/numloops);

	// emptyfn() loop test
	numticks = 0UL;
	numloops = 250000000;
	for (i=0; i<numloops; i++) {
		startclock = clock();
		emptyfn();
		numticks += clock()-startclock;
	}

	secs = (double)numticks/CLOCKS_PER_SEC;
	nanosecs = secs*1E9;
	
	printf("B. emptyfn() loop:\n");
	printf("\t%ld clock ticks (%lf secs) for %ld iterations.\n", numticks, secs, numloops);
	printf("\t%lf clock ticks (%lf nanosecs) for 1 iteration.\n\n", (double)numticks/numloops, nanosecs/numloops);
	
	
	// getpid() loop test
	numticks = 0UL;
	numloops = 250000000;
	for (i=0; i<numloops; i++) {
		startclock = clock();
		syscall(__NR_getpid); // If getpid() called directly, result is cached
		numticks += clock()-startclock;
	}

	secs = (double)numticks/CLOCKS_PER_SEC;
	nanosecs = secs*1E9;
	
	printf("C. getpid() loop:\n");
	printf("\t%ld clock ticks (%lf secs) for %ld iterations.\n", numticks, secs, numloops);
	printf("\t%lf clock ticks (%lf nanosecs) for 1 iteration.\n\n", (double)numticks/numloops, nanosecs/numloops);
}