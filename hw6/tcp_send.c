/** tcp_send.c by William Ho */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

main(int argc, char *argv[]) {
	struct sockaddr_in sockinfo;
	struct hostent *hostinfo;
	int sockfd, port, bufsize = 8192;
	
	int bytestowrite, byteswritten, bufoffset; 
	long totalbyteswritten=0;
	char *buf, *endptr;
	
	struct timeval starttimeval, endtimeval;
	double starttime, endtime, timediff;
	
	if (argc<3) {
		fprintf(stderr,"Usage: %s hostname port <input_stream\n",argv[0]);
		exit(-1);
	}
	
	if ((sockinfo.sin_addr.s_addr=inet_addr(argv[1]))== -1) {
		if (!(hostinfo=gethostbyname(argv[1]))) {
			fprintf(stderr,"Unknown host: %s",argv[1]);
			herror(" ");
			exit(-1);
		}
		memcpy(&sockinfo.sin_addr.s_addr, hostinfo->h_addr_list[0], sizeof sockinfo.sin_addr.s_addr);
	}
	
	port = strtol(argv[2],&endptr,10);
	if (*endptr != '\0') {
		fprintf(stderr,"Invalid port: %s\n",argv[2]);
		exit(-1);
	}
	
	sockinfo.sin_port = htons(port);
	sockinfo.sin_family = AF_INET;
	
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		perror("Failed to establish socket");
		exit(-1);
	}
	
	if (connect(sockfd, (struct sockaddr *) &sockinfo, sizeof sockinfo) < 0) {
		perror("Failed to connect");
		close(sockfd);
		exit(-1);
	}
	
	buf = malloc(bufsize);
	gettimeofday(&starttimeval,NULL);
	starttime = starttimeval.tv_sec+(starttimeval.tv_usec/1000000.0);
	while ((bytestowrite = read(0,buf,bufsize)) > 0) {
		bufoffset = 0;
		totalbyteswritten += (byteswritten = write(sockfd,buf,bytestowrite));

		while (byteswritten < bytestowrite) { // Error or partial write
			if (byteswritten <= 0) {
				fprintf(stderr,"Problem writing to socket: %s\n",strerror(errno));
				exit(-1);
			}
			bufoffset += byteswritten;
			bytestowrite -= byteswritten;
			totalbyteswritten += (byteswritten = write(sockfd,buf+bufoffset,bytestowrite));
		}
	}
	
	if (bytestowrite<0) {
		perror("Failed to write to socket");
		exit(-1);
	}
	
	if (shutdown(sockfd,SHUT_WR) < 0) {
		perror("Error on attempting to close socket");
		exit(-1);
	}
	
	gettimeofday(&endtimeval,NULL);
	endtime = endtimeval.tv_sec+(endtimeval.tv_usec/1000000.0);
	timediff = endtime-starttime;
	
	fprintf(stderr,"%ld bytes sent in %.3f seconds (%f MB/s)\n",totalbyteswritten,timediff,((double)totalbyteswritten)/(1000000*timediff));
}