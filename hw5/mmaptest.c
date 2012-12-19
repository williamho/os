/* mmaptest.c by William Ho */

#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

void sig_handler(int signum);
char * mapfile(int len, int prot, int flags, int fd);
void memdump(char *ptr, int offset, int len, int isfile);
int createfile(char *fname, int len);

main(int argc, char *argv[]) {
	int i, fd, status, flags, numbytes, bytesread;
	int len = 16;
	char part, *cptr, *buf, *tmpbuf;
	char *fname = "testfile.txt";
	struct stat st;
	
	if (argc > 1) {
		for (i=0; i<32; i++)
			signal(i,sig_handler);
	
		printf("Part %c:\n",part = toupper(argv[1][0]));
		switch(part) {
		case 'A':
			fd = createfile(fname,len);
			cptr = mapfile(len,PROT_READ,MAP_SHARED,fd);
			printf("Attempting to write to mapped area.\n");
			cptr[0] = 'a'; // Causes a signal to be handled by sig_handler
			break;
			
		case 'B':
		case 'C':
			buf = malloc(len);
			fd = createfile(fname,len);

			if (part == 'B')
				flags = MAP_SHARED;
			else
				flags = MAP_PRIVATE;
				
			cptr = mapfile(len,PROT_READ|PROT_WRITE,flags,fd);
			
			if (lseek(fd,0,SEEK_SET) < 0) {
				fprintf(stderr,"Error: Could not lseek to beginning of file \"%s\": %s\n",fname,strerror(errno));
				exit(-1);
			}
			if (read(fd,buf,len) < 0) {
				fprintf(stderr,"Error: Could not read from file \"%s\": %s\n",fname,strerror(errno));
				exit(-1);
			}
			printf("Original contents of file:\n");
			memdump(buf,0,len,1);
			
			printf("\nWriting value 0x3D to byte 0 of memory-mapped region.\n");
			cptr[0] = 0x3D;
			if (lseek(fd,0,SEEK_SET) < 0) {
				fprintf(stderr,"Error: Could not lseek to beginning of file \"%s\": %s\n",fname,strerror(errno));
				exit(-1);
			}
			if (read(fd,buf,len) < 0) {
				fprintf(stderr,"Error: Could not read from file \"%s\": %s\n",fname,strerror(errno));
				exit(-1);
			}
			printf("Contents of file after writing to mapped memory:\n");
			memdump(buf,0,len,1);
			
			if (part == 'B')
				printf("\nYes, with MAP_SHARED, the update is immediately visible when accessing the file through the read system call.\n");
			else
				printf("\nNo, with MAP_PRIVATE, the update is not immediately visible when accessing the file through the read system call.\n");
			
			break;
			
		case 'D':
		case 'E':
			len = 8193;
			numbytes = 4;
			buf = malloc(len*2);
			fd = createfile(fname,len);
			
			cptr = mapfile(len*2,PROT_READ|PROT_WRITE,MAP_SHARED,fd);
			if (fstat(fd, &st) < 0) {
				fprintf(stderr,"Error: Could not stat file \"%s\": %s\n",fname,strerror(errno));
				exit(-1);
			}
			printf("Original size of file according to stat: %ld\n",(long)st.st_size);
			
			printf("Writing %d bytes (with value 0x02) to the mmap region at offset %d.\n",numbytes,len);
			for (i=0; i<numbytes; i++)
				cptr[len+i] = 2;
			
			if (fstat(fd, &st) < 0) {
				fprintf(stderr,"Error: Could not stat file \"%s\": %s\n",fname,strerror(errno));
				exit(-1);
			}
			printf("New size of file according to stat: %ld\n",(long)st.st_size);
			
			if ((bytesread = read(fd,buf,len+numbytes)) < 0) {
				fprintf(stderr,"Error: Could not read from file \"%s\": %s\n",fname,strerror(errno));
				exit(-1);
			}
			printf("File dump starting at offset %d (read(2) requested %d bytes and returned %d):\n",len,numbytes,bytesread);
			memdump(buf,len,bytesread,1);
			printf("\nMemory dump starting at offset %d for %d bytes:\n",len,numbytes);
			memdump(cptr,len,numbytes,0);
			printf("\nFor part D: The size of the file does not change.\n");
			
			
			if (part == 'D')
				break;
			
			printf("Seeking to %d bytes past end of file and writing %d bytes (with value 0x3D).\n",numbytes*2,numbytes);
			if (lseek(fd,numbytes*2,SEEK_END) < 0) {
				fprintf(stderr,"Error: Could not lseek to %d bytes past end of file \"%s\": %s\n",numbytes*2,fname,strerror(errno));
				exit(-1);
			}
			
			tmpbuf = malloc(numbytes);
			for (i=0; i<numbytes; i++)
				tmpbuf[i] = 0x3D;
			if (write(fd,tmpbuf,numbytes) <= 0) {
				fprintf(stderr,"Error: Could not write to file \"%s\": %s\n",fname,strerror(errno));
				exit(-1);
			}
			printf("Memory dump starting at offset %d for %d bytes:\n",len,numbytes*4);
			memdump(cptr,len,numbytes*4,0);
			
			if (lseek(fd,len,SEEK_SET) < 0) {
				fprintf(stderr,"Error: Could not lseek to offset %d of file \"%s\": %s\n",len,fname,strerror(errno));
				exit(-1);
			}
			if ((bytesread = read(fd,buf+len,numbytes*4)) < 0) {
				fprintf(stderr,"Error: Could not read from file \"%s\": %s\n",fname,strerror(errno));
				exit(-1);
			}
			printf("File dump starting at offset %d (read(2) requested %d bytes and returned %d):\n",len,numbytes*4,bytesread);
			memdump(buf,len,bytesread,1);
			
			printf("\nFor part E: Yes, the data previously written to the hole are visible in the file.");
			break;
			
		case 'F':
			fd = createfile(fname,len);
			cptr = mapfile(len+10000,PROT_WRITE,MAP_SHARED,fd);
			printf("Attempting to write to memory beyond the current end of file (and crossing a page boundary).\n");
			cptr[len+5000] = 'a'; // Causes a signal to be handled by sig_handler
			break;
			
		case 'G':
			len = 5000;
			fd = createfile(fname,len);
			cptr = mapfile(len,PROT_WRITE,MAP_SHARED,fd);
			
			printf("Truncating file to size 1 byte.\n");
			if (ftruncate(fd,1) < 0) {
				fprintf(stderr,"Error: Could not truncate file \"%s\" to 1 byte: %s\n",fname,strerror(errno));
				exit(-1);
			}
			
			printf("Attempting to write to offset %d of memory-mapped region.\n",len/2);
			cptr[len] = 'a'; // Causes a signal to be handled by sig_handler
			break;
			
		default:
			fprintf(stderr,"Error: First argument should be a character from A to G.\n");
			break;
		}
	}
	else
		fprintf(stderr,"Error: Invoke this program with a single argument: a character from A to G.\n");
}

void sig_handler(int signum) {
	printf("Result: Signal %d (%s) was generated.\n",signum,strsignal(signum));
	exit(0);
}

int createfile(char *fname, int len) {
	int i, fd;
	char *contents = malloc(len);
	for (i=0; i<len; i++)
		contents[i] = 1;
	
	printf("Creating %d-byte file \"%s\" where each byte has the value 0x01.\n",len,fname);
	if ((fd = open(fname,O_RDWR|O_CREAT|O_TRUNC,0666)) < 0) {
		fprintf(stderr,"Error: Could not create file \"%s\": %s\n",fname,strerror(errno));
		exit(-1);
	}
	if (write(fd,contents,len) <= 0) {
		fprintf(stderr,"Error: Could not write %d bytes to file \"%s\": %s\n",len,fname,strerror(errno));
		exit(-1);
	}
	return fd;
}

void memdump(char *ptr, int offset, int len, int isfile) {
	int i;	
	char *cptr = ptr+offset;
	for (i=0; i<len; i++)
		printf("<%02X> ",cptr[i]);
	if (isfile)
		printf("<EOF>");
}

char * mapfile(int len, int prot, int flags, int fd) {
	char *ptr, *protstr, *flagsstr;
	switch (prot) {
	case PROT_READ:
		protstr = "read-only";	break;
	case PROT_WRITE:
		protstr = "write-only"; break;
	case PROT_READ|PROT_WRITE:
		protstr = "read/write";	break;
	}
	
	if (flags == MAP_SHARED)
		flagsstr = "MAP_SHARED";
	else // Assume only shared or private, and no other flags
		flagsstr = "MAP_PRIVATE";
	
	printf("Memory-mapping file with %s access and flag %s.\n",protstr,flagsstr);
	if ((ptr = (char*)mmap(NULL,len,prot,flags,fd,0)) < 0) {
		fprintf(stderr,"Error: Could not mmap file for writing: %s\n",strerror(errno));
		exit(-1);
	}
	
	return ptr;
}