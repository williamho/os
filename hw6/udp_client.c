/** udp_client.c by William Ho */

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
	struct sockaddr_in serverinfo;
	struct hostent *hostinfo;
	struct protoent *tcpproto;
	int sockfd, port, bufsize = 1024;
	int serverlen, bytesread;
	char *buf, *endptr;
	
	if (argc<4) {
		fprintf(stderr,"Usage: %s hostname port request_string\n",argv[0]);
		exit(-1);
	}
	
	if ((serverinfo.sin_addr.s_addr=inet_addr(argv[1]))== -1) {
		if (!(hostinfo=gethostbyname(argv[1]))) {
			fprintf(stderr,"Unknown host: %s",argv[1]);
			herror(" ");
			exit(-1);
		}
		memcpy(&serverinfo.sin_addr.s_addr, hostinfo->h_addr_list[0], sizeof serverinfo.sin_addr.s_addr);
	}
	
	port = strtol(argv[2],&endptr,10);
	if (*endptr != '\0') {
		fprintf(stderr,"Invalid port: %s\n",argv[2]);
		exit(-1);
	}
	
	serverinfo.sin_port = htons(port);
	serverinfo.sin_family = AF_INET;
	
	if ((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		perror("Failed to establish socket");
		exit(-1);
	}

	serverlen = sizeof (struct sockaddr_in);
	if (sendto(sockfd,argv[3],strlen(argv[3]),0,(struct sockaddr *) &serverinfo, serverlen) < 0) {
		perror("Could not send request to server");
		exit(-1);
	}
	
	buf = malloc(bufsize+1);
	if (bytesread = recvfrom(sockfd,buf,bufsize,0,(struct sockaddr *) &serverinfo,&serverlen) < 0) {
		perror("Error in receiving response from server");
		exit(-1);
	}	
	printf("%s",buf);
}