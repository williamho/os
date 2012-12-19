/* copycat.c by William Ho
	Copies contents of user-specified file(s) into output stream.
	* option -o changes the output stream to the specified output file
	* option -b specifies the size of the read/write buffer, in bytes
	* input files specified as "-" (dash/hyphen/minus) are instead taken from standard input
	* if no input files specified, input is taken from standard input
*/

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

main(int argc, char *argv[]) {
	int i, bytesToWrite, bytesWritten;
	int inStream = 0, outStream = 1;
	int bufSize = 1024, bufOffset = 0;
	char c, *buf, *outfileName;
	
	// Evaluate arguments
	while ((c = getopt(argc, argv, "b:o:")) != -1) {
		switch(c) {
			case 'b':
				bufSize = atoi(optarg);
				if (bufSize <= 0) {
					fprintf(stderr,"Error: Argument for option -b must be a positive integer.");
					return -1;
				}
				break;
				
			case 'o':
				outfileName = optarg;
				outStream = open(outfileName,O_WRONLY|O_CREAT|O_TRUNC,0644);
				if (outStream == -1) {
					fprintf(stderr,"Error: File \"%s\" cannot be opened for writing: %s",outfileName,strerror(errno));
					return -1;
				}
				break;
				
			case '?':
				if (optopt == 'b' || optopt == 'o')
					fprintf (stderr, "Error: Option -%c requires an argument.\n", optopt);
				else 
					fprintf (stderr, "Error: Unknown option `-%c'.\n", optopt);
				return -1;
				break;
		}
	}
	
	buf = malloc(bufSize);
	if (buf == NULL) {
		fprintf(stderr,"Error: %d bytes of memory could not be allocated for the buffer.",bufSize);
		return -1;
	}
	
	if (optind == argc)	// No input files specified, read from standard input in loop below.
		argv[--optind] = "-";
	
	for (i = optind; i < argc; i++) {
		bufOffset = 0;
	
		if (strcmp(argv[i],"-") != 0) {
			inStream = open(argv[i],O_RDONLY);
			if (inStream == -1) {
				fprintf(stderr,"Error: File \"%s\" cannot be opened for reading: %s\n",argv[i],strerror(errno));
				return -1;
			}
		}
		else
			inStream = 0;
			
		while ((bytesToWrite = read(inStream,buf,bufSize)) > 0) {
			bytesWritten = write(outStream,buf,bytesToWrite);
			if (bytesWritten <= 0) {
				fprintf(stderr,"Error: Problem writing to file \"%s\": %s\n",outfileName,strerror(errno));
				return -1;
			}
			while (bytesWritten < bytesToWrite) { // Partial write
				bufOffset += bytesWritten;
				bytesToWrite -= bytesWritten;
				bytesWritten = write(outStream,buf+bufOffset,bytesToWrite);
			}
		}
		
		if (inStream != 0)
			close(inStream);
			
		if (bytesToWrite < 0) {
			fprintf(stderr,"Error: Problem reading from file \"%s\": %s\n",argv[i],strerror(errno));
			return -1;
		}
	}
	
	return 0;
}
