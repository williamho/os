/* wish.c - William's Shell, by William Ho */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

#define MAX_STRLEN 1024
#define MAX_TOKENS 1024

main(int argc, char *argv[]) {
	char c, *line, *tok, *fname, **argv2, **redir;
	int i, fdreplace, linelen, status, flags, stdin2, stdout2, stderr2, fd;
	struct rusage ru;
	size_t maxlinelen = MAX_STRLEN;
	clock_t starttime, endtime;
	
	if (argv[1] != NULL) {	// If being invoked as a script interpreter
		if ((fd = open(argv[1],O_RDONLY)) < 0) {
			fprintf(stderr,"Error: File \"%s\" cannot be opened for reading: %s\n",argv[1],strerror(errno));
			exit(-1);
		}
		if (dup2(fd,0) < 0) {
			fprintf(stderr,"Error: Cannot dup2 file descriptor for \"%s\": %s\n",argv[1],strerror(errno));
			exit(-1);
		}
		if (close(fd) < 0) {
			fprintf(stderr,"Error: Cannot close duplicate file descriptor for \"%s\": %s\n",argv[1],strerror(errno));
			exit(-1);
		}
	}
	
	line = (char *) malloc(MAX_STRLEN+1);
	if (line == NULL) {
		fprintf(stderr,"Error: Could not allocate %d bytes for a char* buffer: %s\n",MAX_STRLEN+1,strerror(errno));
		exit(-1);
	}
	
	argv2 = (char **) malloc((MAX_TOKENS+1)*sizeof(char*));
	if (argv2 == NULL) {
		fprintf(stderr,"Error: Could not allocate %d bytes for a char** buffer: %s\n",(MAX_TOKENS+1)*sizeof(char*),strerror(errno));
		exit(-1);
	}
	
	while ((linelen = getline(&line,&maxlinelen,stdin)) != -1) {
		if (line[0] == '#' || linelen == 1)
			continue;

		line[linelen-1] = '\0'; // Remove trailing newline
		
		redir = NULL;
		tok = strtok(line," \t");
		for (i=0; tok!=NULL && i<=MAX_TOKENS; i++) {
			if (redir == NULL && (tok[0] == '<' || tok[0] == '>' || (tok[0] == '2' && tok[1] == '>')) ) {
				argv2[i++] = NULL;
				redir = &argv2[i];
			}
			argv2[i] = tok;
			tok = strtok(NULL," \t");
		}
		if (i>MAX_TOKENS) {
			fprintf(stderr,"Error: Too many tokens! Maximum is %d.",MAX_TOKENS);
			exit(-1);
		}
		argv2[i] = NULL;
		
		printf("Executing command %s",argv2[0]);
		if (argv2[1] != NULL) {
			printf(" with arguments \"");
			for (i=1; argv2[i] != NULL; i++) {
				printf("%s",argv2[i]);
				if (argv2[i+1] != NULL)
					putchar(' ');
			}
			putchar('\"');
		}
		printf("\n_____\n");
		
		switch(fork()) {
		case -1:
			fprintf(stderr,"Error: Fork failed: %s\n",strerror(errno));
			exit(-1);
			break;
		case 0:
			if (redir != NULL) {
				for (i=0; redir[i]!=NULL; i++) {
					switch(redir[i][0]) {
					case '<':
						flags = O_RDONLY;
						fname = &redir[i][1];
						fdreplace = 0;
						break;
					case '>':
						if (redir[i][1] == '>') {
							flags = O_WRONLY|O_CREAT|O_APPEND;
							fname = &redir[i][2];
						}
						else {
							flags = O_WRONLY|O_CREAT|O_TRUNC;
							fname = &redir[i][1];
						}
						fdreplace = 1;
						break;
					case '2':
						if (redir[i][2] == '>') {
							flags = O_WRONLY|O_CREAT|O_APPEND;
							fname = &redir[i][3];
						}
						else {
							flags = O_WRONLY|O_CREAT|O_TRUNC;
							fname = &redir[i][2];
						}
						fdreplace = 2;
						break;
					default:
						fprintf(stderr,"Error: Invalid redirection operation: \"%s\"\n",redir[i]);
						exit(-1);
						break;
					}
					
					if (fname[0] == '\0') {
						fprintf(stderr,"Error: Filenames for redirection operations cannot be null.\n");
						exit(-1);
					}
					if ((fd = open(fname,flags,0666)) < 0) {
						fprintf(stderr,"Error: File \"%s\" cannot be opened for %s: %s\n",fname,fdreplace?"writing":"reading",strerror(errno));
						exit(-1);
					}
					if (dup2(fd,fdreplace) < 0) {
						fprintf(stderr,"Error: Cannot dup2 file descriptor for file \"%s\": %s\n",fname,strerror(errno));
						exit(-1);
					}
					if (close(fd) < 0) {
						fprintf(stderr,"Error: Cannot close duplicate file descriptor for file \"%s\": %s\n",fname,strerror(errno));
						exit(-1);
					}
				}
			}
			execvp(argv2[0],argv2);
			fprintf(stderr,"Error: Command %s failed: %s\n",argv2[0],strerror(errno));
			exit(-1);
			break;
		default:
			starttime = clock();
			if (wait3(&status,0,&ru) == -1) {
				fprintf(stderr,"Error: Wait failed: %s",strerror(errno));
				exit(-1);
			}
			endtime = clock();
			printf("\nCommand %s ",argv2[0]);
			if (WIFEXITED(status)) 
				printf("exited with return code %d\n",WEXITSTATUS(status));
			else if (WIFSIGNALED(status))
				printf("terminated with signal %d\n", WTERMSIG(status));
			else if (WIFSTOPPED(status))
				printf("stopped with signal %d\n", WSTOPSIG(status));
			else if (WIFCONTINUED(status))
				printf("continued\n");
			else
				printf("exited with the raw status: %d\n",status);

			printf("Process consumed %.6fs real, %ld.%.6ds user, %ld.%.6ds system time\n_____\n",
					((double)(endtime-starttime))/CLOCKS_PER_SEC,ru.ru_utime.tv_sec,ru.ru_utime.tv_usec,ru.ru_stime.tv_sec,ru.ru_stime.tv_usec);
			break;
		}
	}
	printf("End of file\n");
	return 0;
}
