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

#define client_PORT "25965" // the port client will be connecting to

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
	int sockfd, numbytes;
	char send_buf[MAXDATASIZE] = {0};
	char get_buf[MAXDATASIZE] = {0};
	char get_buf2[MAXDATASIZE] = {0};
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	// 输入为<username1>或 <username1> <username2> <transfer amount>
	if (!((argc == 2) || (argc == 4))) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo("127.0.0.1", client_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	//printf("client: connecting to %s\n", s);
	printf("The client is up and running.\n");

	freeaddrinfo(servinfo); // all done with this structure


	if (argc == 2){ //search
		// send to serverM
		printf("%s sent a balance enquiry request to the main server.\n", argv[1]);
		strcpy (send_buf, argv[1]);
		strcat(send_buf, " NULL");
		strcat(send_buf, " NULL");
		//printf("%s\n", send_buf);

		if (send(sockfd, send_buf, sizeof(send_buf), 0) == -1)
			perror("send");

		// receive from serverM

		if ((numbytes = recv(sockfd, get_buf, MAXDATASIZE-1, 0)) == -1) {
		    perror("recv");
		    exit(1);
		}
		//get_buf[numbytes] = '\0';
		//printf("client: received '%s'\n",get_buf);
		char str1[MAXDATASIZE]={0}, str2[MAXDATASIZE]={0};
		sscanf(get_buf, "%s%s", str1, str2);
		//printf("%s %s %s\n", get_buf, str1, str2);
		printf("The current balance of %s is : %s txcoins.\n", argv[1], str1);

	}
	else if (argc == 4) { // transfer
		//TODO
		printf("%s has requested to transfer %s txcoins to %s.\n", argv[1], argv[3], argv[2]);
		strcpy (send_buf, argv[1]);
		strcat (send_buf, " ");
		strcat (send_buf, argv[2]);
		strcat (send_buf, " ");
		strcat (send_buf, argv[3]);
		//printf("%s\n", send_buf);

		if (send(sockfd, send_buf, sizeof(send_buf), 0) == -1)
			perror("send");

		// receive from serverM
		if ((numbytes = recv(sockfd, get_buf2, MAXDATASIZE-1, 0)) == -1) {
		    perror("recv");
		    exit(1);
		}
		//get_buf[numbytes] = '\0';
		//printf("client: received '%s'\n",get_buf);
		char str1[MAXDATASIZE]={0}, str2[MAXDATASIZE]={0}, str3[MAXDATASIZE] = {0};
		sscanf(get_buf2, "%s%s%s", str1, str2, str3);
		if(!strcmp(str1, "NOTENOUGH")) {
			// 钱不够
			printf("%s was unable to transfer %s txcoins to "
			"%s because of insufficient balance.\n"
			"The current balance of %s is : %s txcoins.\n", argv[1], argv[3], argv[2], argv[1], str2);
		}
		else if((!strcmp(str1, "NULL")) && (strcmp(str2, "NULL"))) {
			// A 不存在
			printf("Unable to proceed with the transaction as %s is not part of the network.\n", argv[1]);
		}
		else if((strcmp(str1, "NULL")) && (!strcmp(str2, "NULL"))) {
			// B 不存在
			printf("Unable to proceed with the transaction as %s is not part of the network.\n", argv[2]);
		}
		else if((!strcmp(str1, "NULL")) && (!strcmp(str2, "NULL"))) {
			// AB 不存在
			printf("Unable to proceed with the transaction as %s and %s are not part of the network.\n", argv[1], argv[2]);
		}
		else {
			//transfer OK
			printf("%s successfully transferred %s txcoins to %s.\n"
			"The current balance of %s is : %s txcoins.\n", argv[1], argv[3], argv[2], argv[1], str3);
		}

	}


	close(sockfd);

	return 0;
}
