/*
  basecode from https://beej.us/guide/bgnet/examples/client.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define monitor_PORT "26965" // the port client will be connecting to

#define MAXDATASIZE 10000 // max number of bytes we can get at once



// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd2, numbytes;
	char send_buf[MAXDATASIZE];
	char get_buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	// 输入为<username1>或 <username1> <username2> <transfer amount>
	if (!((argc == 2) || (argc == 4))) {
	    fprintf(stderr,"usage: monitor hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo("127.0.0.1", monitor_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd2 = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("monitor: socket");
			continue;
		}

		if (connect(sockfd2, p->ai_addr, p->ai_addrlen) == -1) {
			perror("monitor: connect");
			close(sockfd2);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "monitor: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	//printf("monitor: connecting to %s\n", s);
	printf("The monitor is up and running.\n");

	freeaddrinfo(servinfo); // all done with this structure


	if (argc == 2){ //TXLIST
		// send to serverM

		if (send(sockfd2, argv[1], sizeof(argv[1]), 0) == -1)
			perror("send");
		printf("Monitor sent a sorted list request to the main server.\n");

		// receive from serverM
		if ((numbytes = recv(sockfd2, get_buf, MAXDATASIZE-1, 0)) == -1) {
		    perror("recv");
		    exit(1);
		}
		get_buf[numbytes] = '\0';
		//printf("monitor: received '%s'\n",get_buf);

		if(!strcmp(get_buf, "print success!")) {
			printf("Successfully received a sorted list of transactions from the main server.\n");
		}

	}


	close(sockfd2);

	return 0;
}
