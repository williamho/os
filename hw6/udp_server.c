/** udp_server.c by William Ho */

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
	int sockfd, readsock, port, clientlen, bufsize=1024;
	
	int bytesread, bytesreadfromshell;
	char *buf, *endptr, *outbuf;
	char *invalid = "Invalid request.\n";
	FILE *shelloutput;
	int shellfd;
	
	struct timeval starttimeval, endtimeval;
	double starttime, endtime, timediff;
	
	if (argc<2) {
		fprintf(stderr,"Usage: %s port\n",argv[0]);
		exit(-1);
	}
	
	buf = malloc(bufsize+1);
	buf[bufsize] = '\0';
	outbuf = malloc(bufsize+1);
	port = strtol(argv[1],&endptr,10);
	if (*endptr != '\0') {
		fprintf(stderr,"Invalid port: %s\n",argv[2]);
		exit(-1);
	}
	
	sockinfo.sin_addr.s_addr=INADDR_ANY;
	sockinfo.sin_port = htons(port);
	sockinfo.sin_family = AF_INET;
	
	if ((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		perror("Failed to establish socket");
		exit(-1);
	}
	
	if (bind(sockfd,(struct sockaddr *) &sockinfo, sizeof sockinfo) < 0) {
		perror("Failed to bind socket");
		close(sockfd);
		exit(-1);
	}
	
	printf("Now listening to incoming UDP connections on port %d.\n",port);
	clientlen = sizeof (struct sockaddr_in);
	while(1) {
		memset(buf,0,bufsize);
		if ((bytesread = recvfrom(sockfd,buf,bufsize,0,(struct sockaddr *)&clientinfo,&clientlen)) < 0) {
			perror("recvfrom");
			exit(-1);
		}
		printf("Received datagram with contents: %s\n",buf);
		if (strcmp(buf,"UPTIME")==0) {
			shelloutput = popen("uptime","r");
			shellfd = fileno(shelloutput);
			if ((bytesreadfromshell=read(shellfd,outbuf,bufsize))<0)
				perror("Failed to read output of 'uptime' command");
			
			if (sendto(sockfd,outbuf,bytesreadfromshell,0,(struct sockaddr *)&clientinfo,clientlen)<0)
				perror("Could not reply to UPTIME request");
		}
		else if (strcmp(buf,"DATE")==0) {
			shelloutput = popen("date","r");
			shellfd = fileno(shelloutput);
			if ((bytesreadfromshell=read(shellfd,outbuf,bufsize))<0)
				perror("Failed to read output of 'date' command");
			
			if (sendto(sockfd,outbuf,bytesreadfromshell,0,(struct sockaddr *)&clientinfo,clientlen)<0)
				perror("Could not reply to DATE request");
		}
		else {
			if (sendto(sockfd,invalid,strlen(invalid),0,(struct sockaddr *)&clientinfo,clientlen)<0)
				perror("Could not reply to invalid request");
		}
	}
}