/** tcp_recv.c by William Ho */

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
	struct sockaddr_in sockinfo, clientinfo;
	struct hostent *he;
	int sockfd, readsock, port, clientlen, bufsize=8192;
	
	int bytesread; 
	long totalbytesread=0;
	char *buf, *endptr;
	char clientip[bufsize];
	
	struct timeval starttimeval, endtimeval;
	double starttime, endtime, timediff;
	
	if (argc<2) {
		fprintf(stderr,"Usage: %s port >output_file\n",argv[0]);
		exit(-1);
	}
	
	buf = malloc(bufsize);
	port = strtol(argv[1],&endptr,10);
	if (*endptr != '\0') {
		fprintf(stderr,"Invalid port: %s\n",argv[2]);
		exit(-1);
	}
	
	sockinfo.sin_addr.s_addr=INADDR_ANY;
	sockinfo.sin_port = htons(port);
	sockinfo.sin_family = AF_INET;
	
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		perror("Failed to establish socket");
		exit(-1);
	}
	
	if (bind(sockfd,(struct sockaddr *) &sockinfo, sizeof sockinfo) < 0) {
		perror("Failed to bind socket");
		close(sockfd);
		exit(-1);
	}
	
	if (listen(sockfd, 1) < 0) {
		perror("Failed to listen");
		close(sockfd);
		exit(-1);
	}
	
	fprintf(stderr,"Listening for incoming connections on port %d.\n",port);
	
	clientlen = sizeof clientinfo;
	if ((readsock = accept(sockfd, (struct sockaddr *) &clientinfo, &clientlen)) < 0) {
		perror("Failed to accept connection");
		close(sockfd);
		exit(-1);
	}
	
	fprintf(stderr,"Incoming connection from: %s ",inet_ntoa(clientinfo.sin_addr));
	if ((he = gethostbyaddr((void*)&clientinfo.sin_addr,clientlen,clientinfo.sin_family)) != 0)
		fprintf(stderr,"(%s) ",he->h_name);
	fprintf(stderr,"at port %d\n-----\n",clientinfo.sin_port);
	
	gettimeofday(&starttimeval,NULL);
	starttime = starttimeval.tv_sec+(starttimeval.tv_usec/1000000.0);
	while((bytesread=read(readsock,buf,bufsize))>0) {
		write(1,buf,bytesread);
		totalbytesread += bytesread;
	}
	
	if (bytesread < 0) {
		perror("Failed to read from socket");
		exit(-1);
	}
	if (close(sockfd) < 0) {
		perror("Failed to close sockfd");
		exit(-1);
	}
	if (close(readsock) < 0) {
		perror("Failed to close readsock");
		exit(-1);
	}
	timediff = ((double)(clock()-starttime))/CLOCKS_PER_SEC;	
	
	gettimeofday(&endtimeval,NULL);
	endtime = endtimeval.tv_sec+(endtimeval.tv_usec/1000000.0);
	timediff = endtime-starttime;
	
	fprintf(stderr,"%ld bytes received in %.3f seconds (%f MB/s)\n",totalbytesread,timediff,((double)totalbytesread)/(1000000*timediff));
}