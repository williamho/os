#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

long totalbyteswritten = 0;
int filesprocessed = 0;

void redirectpipe(int *fds, int side);
void int_handler(int sn);

main(int argc, char *argv[]) {
	int i, bytestowrite, byteswritten, status;
	int infile = 0, bufsize = 2048, bufoffset = 0, fds1[2], fds2[2];
	char *buf;
	pid_t morepid;
	sigset_t mask;
	
	if (argc < 3) {
		fprintf(stderr,"Error: Not enough input arguments! A pattern and at least one input file must be specified.\n");
		exit(-1);
	}
	
	buf = malloc(bufsize);
	if (buf == NULL) {
		fprintf(stderr,"Error: %d bytes of memory could not be allocated for the read/write buffer.\n",bufsize);
		exit(-1);
	}
	
	for (i=2; i<argc; i++) {
		if (pipe(fds1) < 0) {
			fprintf(stderr,"Error: Cannot create pipe between 'cat' and 'grep': %s\n",strerror(errno));
			exit(-1);
		}
		
		switch(morepid = fork()) {
		case -1:
			fprintf(stderr,"Error: Fork failed: %s\n",strerror(errno));
			exit(-1);
			break;
		case 0:	
			redirectpipe(fds1,0);
			
			if (pipe(fds2) < 0) {
				fprintf(stderr,"Error: Cannot create pipe between 'grep' and 'more': %s\n",strerror(errno));
				exit(-1);
			}
			// 'more' and 'grep' are set to ignore SIGINT
			if (sigemptyset(&mask) < 0) {
				fprintf(stderr,"Error: Could not create empty signal set: %s\n",strerror(errno));
				exit(-1);
			}
			if (sigaddset(&mask,SIGINT) < 0) {
				fprintf(stderr,"Error: Could not add SIGINT to signal set: %s\n",strerror(errno));
				exit(-1);
			}
			if (sigprocmask(SIG_BLOCK,&mask,NULL) < 0) {
				fprintf(stderr,"Error: Child process' signal masks could not be changed: %s\n",strerror(errno));
				exit(-1);
			}
			switch(fork()) {
			case -1:
				fprintf(stderr,"Error: Fork failed: %s\n",strerror(errno));
				exit(-1);
				break;
			case 0: // more
				redirectpipe(fds2,1);
				execlp("grep","grep",argv[1],NULL);
				fprintf(stderr,"Error: Failed to exec 'grep': %s\n",strerror(errno));
				exit(-1);
				break;
			default: // grep
				redirectpipe(fds2,0);
				execlp("more","more",NULL);
				fprintf(stderr,"Error: Failed to exec 'more': %s\n",strerror(errno));
				exit(-1);
				break;
			}
			break;
		default: // cat
			signal(SIGINT,int_handler);
			if (close(fds1[0]) < 0) {
				fprintf(stderr,"Error: Cannot close file descriptor for read side of pipe: %s\n",strerror(errno));
				exit(-1);
			}
			if (strcmp(argv[i],"-") != 0) { // Does this need to be supported??
				infile = open(argv[i],O_RDONLY);
				if (infile < 0) {
					fprintf(stderr,"Error: File \"%s\" cannot be opened for reading: %s\n",argv[i],strerror(errno));
					exit(-1);
				}
			}
			else
				infile = 0;
			
			while ((bytestowrite = read(infile,buf,bufsize)) > 0) {
				bufoffset = 0;
				totalbyteswritten += (byteswritten = write(fds1[1],buf,bytestowrite));

				while (byteswritten < bytestowrite) { // Error or partial write
					if (byteswritten <= 0) {
						fprintf(stderr,"Error: Problem writing to pipe between 'cat' and 'grep': %s\n",strerror(errno));
						exit(-1);
					}
					bufoffset += byteswritten;
					bytestowrite -= byteswritten;
					totalbyteswritten += (byteswritten = write(fds1[1],buf+bufoffset,bytestowrite));
				}
			}
			if (bytestowrite < 0) {
				fprintf(stderr,"Error: Problem reading from file \"%s\": %s\n",argv[i],strerror(errno));
				exit(-1);
			}
			if (close(fds1[1]) < 0) {
				fprintf(stderr,"Error: Could not close pipe between 'cat' and 'grep': %s\n",strerror(errno));
				exit(-1);
			}
			filesprocessed++;
			
			if (infile != 0 && close(infile)<0) {
				fprintf(stderr,"Error: Could not close input file \"%s\": %s\n",argv[i],strerror(errno));
				exit(-1);
			}
			if (waitpid(morepid,&status,WUNTRACED) < 1) { // Wait for 'more' to exit before going to next file
				fprintf(stderr,"Error: Wait failed: %s",strerror(errno));
				exit(-1);
			}
			break;
		}
	}
	return 0;
}

// Redirect stdin or stdout to corresponding side of pipe, closing the dangling file descriptors
void redirectpipe(int *fds, int side) {
	if (dup2(fds[side],side) < 0) {
		fprintf(stderr,"Error: Cannot dup2 file descriptor for %s side of pipe: %s\n",side?"write":"read",strerror(errno));
		exit(-1);
	}
	if (close(fds[side]) < 0) {
		fprintf(stderr,"Error: Cannot close duplicate file descriptor for %s side of pipe: %s\n",side?"write":"read",strerror(errno));
		exit(-1);
	}
	if (close(fds[!side]) < 0) {
		fprintf(stderr,"Error: Cannot close file descriptor for %s side of pipe: %s\n",(!side)?"write":"read",strerror(errno));
		exit(-1);
	}
}

void int_handler(int sn) {
	fprintf(stderr,"%d files (total %ld bytes) processed.\n",filesprocessed,totalbyteswritten);
}