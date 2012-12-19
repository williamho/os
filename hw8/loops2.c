#include <stdio.h>
#include <time.h>
#include <sys/syscall.h>

#define TIMESPEC_TO_NANO(x) ((x).tv_sec*1000000000 + (x).tv_nsec)
#define NANO_TO_SEC(x) ((double)(x)/1000000000)
#define PROCESSOR_HZ 3160000000UL // 3.16GHz. Note: This value is specific to this computer
#define SEC_TO_CYCLES(x) ((x)*PROCESSOR_HZ)

void emptyfn() {}
struct timespec tdiff(struct timespec *start, struct timespec *end, struct timespec *out);

main() {
	unsigned long i, numticks, numloops;
	struct timespec time1, time2, timediff;
	double secs;
	unsigned long nanosecs;
	
	// Empty loop test
	numloops = 5000000000;
	clock_gettime(CLOCK_REALTIME, &time1);
	for (i=0; i<numloops; i++);
	clock_gettime(CLOCK_REALTIME, &time2);
	
	tdiff(&time1,&time2,&timediff);
	nanosecs = TIMESPEC_TO_NANO(timediff);
	secs = NANO_TO_SEC(nanosecs);
	numticks = SEC_TO_CYCLES(secs);
	
	printf("A. Empty loop:\n");
	printf("\t%ld clock ticks (%lf secs) for %ld iterations.\n", numticks, secs, numloops);
	printf("\t%ld clock ticks (%ld nanosecs) for 1 iteration.\n\n", numticks/numloops, nanosecs/numloops);

	// emptyfn() loop test
	nanosecs = 0;
	numloops = 25000000;
	for (i=0; i<numloops; i++) {
		clock_gettime(CLOCK_REALTIME, &time1);
		emptyfn();
		clock_gettime(CLOCK_REALTIME, &time2);
		tdiff(&time1,&time2,&timediff);
		nanosecs += TIMESPEC_TO_NANO(timediff);
	}

	secs = NANO_TO_SEC(nanosecs);
	numticks = SEC_TO_CYCLES(secs);
	
	printf("B. emptyfn() loop:\n");
	printf("\t%ld clock ticks (%lf secs) for %ld iterations.\n", numticks, secs, numloops);
	printf("\t%ld clock ticks (%ld nanosecs) for 1 iteration.\n\n", numticks/numloops, nanosecs/numloops);
	
	
	// getpid() loop test
	nanosecs = 0;
	numloops = 25000000;
	for (i=0; i<numloops; i++) {
		clock_gettime(CLOCK_REALTIME, &time1);
		syscall(__NR_getpid); // If getpid() called directly, result is cached
		clock_gettime(CLOCK_REALTIME, &time2);
		tdiff(&time1,&time2,&timediff);
		nanosecs += TIMESPEC_TO_NANO(timediff);
	}

	secs = NANO_TO_SEC(nanosecs);
	numticks = SEC_TO_CYCLES(secs);
	
	printf("C. getpid() loop:\n");
	printf("\t%ld clock ticks (%lf secs) for %ld iterations.\n", numticks, secs, numloops);
	printf("\t%ld clock ticks (%ld nanosecs) for 1 iteration.\n\n", numticks/numloops, nanosecs/numloops);
}

struct timespec tdiff(struct timespec *start, struct timespec *end, struct timespec *out) {
	if ((end->tv_nsec-start->tv_nsec)<0) {
		out->tv_sec = end->tv_sec-start->tv_sec-1;
		out->tv_nsec = 1000000000+end->tv_nsec-start->tv_nsec;
	} else {
		out->tv_sec = end->tv_sec-start->tv_sec;
		out->tv_nsec = end->tv_nsec-start->tv_nsec;
	}
}
