/*
	basecode from https://beej.us/guide/bgnet/examples/server.c
	and  https://beej.us/guide/bgnet/examples/talker.c
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
#include <time.h> // random func

#define client_PORT "25965"  // the port users will be connecting to
#define monitor_PORT "26965"
#define UDP_PORT "24965"

#define A_PORT "21965"
#define B_PORT "22965"
#define C_PORT "23965"

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAXDATASIZE 50000 // max number of bytes we can get at once
#define MAXBUFSIZE 10000 // max number of bytes we can get at once

#define FILE_NAME "txchain.txt" // 存储所有records

char get_buf[MAXDATASIZE] = {0};
char state[MAXBUFSIZE] = {0};

struct Transaction_2{
  char id[MAXBUFSIZE];
  char sender[MAXBUFSIZE];
  char receiver[MAXBUFSIZE];
  char amount[MAXBUFSIZE];
};

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char* itoa(int value, char* result, int base) {
		// check that the base if valid
		if (base < 2 || base > 36) { *result = '\0'; return result; }

		char* ptr = result, *ptr1 = result, tmp_char;
		int tmp_value;

		do {
			tmp_value = value;
			value /= base;
			*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
		} while ( value );

		// Apply negative sign
		if (tmp_value < 0) *ptr++ = '-';
		*ptr-- = '\0';
		while(ptr1 < ptr) {
			tmp_char = *ptr;
			*ptr--= *ptr1;
			*ptr1++ = tmp_char;
		}
		return result;
	}

int initial_TCP (char PORT[]) {
	int sockfd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	int yes=1;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	return sockfd;
}

int initial_UDP (char PORT[]) {
	int sockfd_U; // listen on sock_fd, new connection on new_fd
	struct addrinfo hints_U, *servinfo_U, *p_U;
	int rv_U;

	memset(&hints_U, 0, sizeof hints_U);
	hints_U.ai_family = AF_UNSPEC;
	hints_U.ai_socktype = SOCK_DGRAM; // UDP

	if ((rv_U = getaddrinfo("127.0.0.1", PORT, &hints_U, &servinfo_U)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_U));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p_U = servinfo_U; p_U != NULL; p_U = p_U->ai_next) {
		if ((sockfd_U = socket(p_U->ai_family, p_U->ai_socktype,
				p_U->ai_protocol)) == -1) {
			perror("serverABC: socket");
			continue;
		}

		break;
	}


	if (p_U == NULL)  {
		fprintf(stderr, "serverABC: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo_U); // all done with this structure

	return sockfd_U;
}

char * recvFclient (int new_fd){
	int numbytes;
	memset(get_buf, 0x00, sizeof(char) * MAXDATASIZE); // set get_buf to NULL
	if ((numbytes = recv(new_fd, get_buf, MAXDATASIZE-1, 0)) == -1) {
			perror("recv");
			exit(1);
	}

	return get_buf;

}

void send2C (int sockfd_U, char* str, struct addrinfo *p_C, char* port){
	int numbytes;
	if ((numbytes=sendto(sockfd_U, str, strlen(str), 0,
	 p_C -> ai_addr, p_C -> ai_addrlen)) == -1) {
		 perror("sendto");
		 exit(1);
	 }
	 printf("The main server sent a request to server %s.\n", port);
}

char * recvFC (int sockfd_U, char* port) {
	int numbytes;
	socklen_t sin_size_U;
	struct sockaddr_storage their_addr_U;
	sin_size_U = sizeof their_addr_U;

	memset(get_buf, 0x00, sizeof(char) * MAXDATASIZE); // set get_buf to NULL

	if ((numbytes = recvfrom(sockfd_U, get_buf, MAXDATASIZE-1 , 0,
	 (struct sockaddr *)&their_addr_U, &sin_size_U)) == -1) {
	 perror("recvfrom");
	 exit(1);
	 }

	 printf("The main server received transactions from Server %s using UDP over port 24965.\n", port);

	 return get_buf;
}

void send2client (int new_fd, char* str){
	if (send(new_fd, str, strlen(str), 0) == -1)
		perror("send");
	close(new_fd);
}

char * get_balance(char* name, int sockfd_U, struct addrinfo *p_A,
	struct addrinfo *p_B, struct addrinfo *p_C) {

	int balance = 1000;
	memset(state, 0x00, sizeof(char) * MAXBUFSIZE); // set state to NULL

	// ask C
	send2C(sockfd_U, name, p_C, "C");
	// get from C
	char m1[MAXDATASIZE];
	strcpy(m1, recvFC(sockfd_U, "C"));
	//printf("recv balance from C:%s\n", m1);

	// ask B
	send2C(sockfd_U, name, p_B, "B");
	// get from B
	char m2[MAXDATASIZE];
	strcpy(m2, recvFC(sockfd_U, "B"));
	//printf("recv balance from B:%s\n", m2);

	// ask A
	send2C(sockfd_U, name, p_A, "A");
	// get from A
	char m3[MAXDATASIZE];
	strcpy(m3, recvFC(sockfd_U, "A"));
	//printf("recv balance from A:%s\n", m3);

	if(!(strcmp(m1, "NULL") || strcmp(m2, "NULL") || strcmp(m3, "NULL"))){
		// name not exist
		strcpy(state, "NULL");
	}
	else{
		// name exists
		int money1 = atoi(m1);
		int money2 = atoi(m2);
		int money3 = atoi(m3);
		//printf("recv $$ from C:%d\n", money1);
		balance = balance + money1 + money2 + money3;
		itoa(balance, state, 10);
	}


	return state;
}

int get_id(int sockfd_U, struct addrinfo *p_A, struct addrinfo *p_B,
	struct addrinfo *p_C){
		char str[MAXBUFSIZE] = "ID";
		char id1[MAXDATASIZE], id2[MAXDATASIZE], id3[MAXDATASIZE];
		// ask A
		send2C(sockfd_U, str, p_A, "A");
		strcpy(id1, recvFC(sockfd_U, "A"));
		int d1 = atoi(id1);

		// ask B
		send2C(sockfd_U, str, p_B, "B");
		strcpy(id2, recvFC(sockfd_U, "B"));
		int d2 = atoi(id2);

		// ask C
		send2C(sockfd_U, str, p_C, "C");
		strcpy(id3, recvFC(sockfd_U, "C"));
		int d3 = atoi(id3);

		// calculate id
		int d_max = d1+d2+d3;
		return d_max;

}


int main(void)
{
	int sockfd, sockfd2, new_fd, new_fd2;
	socklen_t sin_size, sin_size2;
	struct sockaddr_storage their_addr, their_addr2; // connector's address information
	char s[INET6_ADDRSTRLEN];
	char s2[INET6_ADDRSTRLEN];

	// for UDP
	int sockfd_U, fd_U; // listen on sock_fd, new connection on new_fd
	//socklen_t sin_size_U;
	//struct sockaddr_storage their_addr_U; // connector's address information
	char s_U[INET6_ADDRSTRLEN];

	// self define
	int numbytes;
	//char idtr[MAXDATASIZE];
	char name1[MAXBUFSIZE]; // sender name
	char name2[MAXBUFSIZE]; // receiver name
	int amount; // transfer amount


	/////////////////////// initial client connect
	sockfd = initial_TCP(client_PORT);

	/////////////////////// initial monitor connect
	sockfd2 = initial_TCP(monitor_PORT);

	/////////////////////// initial UDP connect
	sockfd_U = initial_UDP(UDP_PORT);

	/////////////////////// initial serverA connect
	int sockfd_A, fd_A;
	struct addrinfo hints_A, *servinfo_A, *p_A;
	int rv_A;

	memset(&hints_A, 0, sizeof hints_A);
	hints_A.ai_family = AF_UNSPEC;
	hints_A.ai_socktype = SOCK_DGRAM;

	if ((rv_A = getaddrinfo("127.0.0.1", A_PORT, &hints_A, &servinfo_A)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_A));
		return 1;
	}

	for(p_A = servinfo_A; p_A != NULL; p_A = p_A->ai_next) {
		if ((sockfd_A = socket(p_A->ai_family, p_A->ai_socktype,
				p_A->ai_protocol)) == -1) {
			perror("serverC: socket");
			continue;
		}
		break;
	}

	if (p_A == NULL)  {
		fprintf(stderr, "serverC: failed to bind\n");
		return 2;
	}

	/////////////////////// initial serverB connect
	int sockfd_B, fd_B;
	struct addrinfo hints_B, *servinfo_B, *p_B;
	int rv_B;

	memset(&hints_B, 0, sizeof hints_B);
	hints_B.ai_family = AF_UNSPEC;
	hints_B.ai_socktype = SOCK_DGRAM;

	if ((rv_B = getaddrinfo("127.0.0.1", B_PORT, &hints_B, &servinfo_B)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_B));
		return 1;
	}

	for(p_B = servinfo_B; p_B != NULL; p_B = p_B->ai_next) {
		if ((sockfd_B = socket(p_B->ai_family, p_B->ai_socktype,
				p_B->ai_protocol)) == -1) {
			perror("serverB: socket");
			continue;
		}
		break;
	}

	if (p_B == NULL)  {
		fprintf(stderr, "serverC: failed to bind\n");
		return 2;
	}

	/////////////////////// initial serverC connect
	int sockfd_C, fd_C;
	struct addrinfo hints_C, *servinfo_C, *p_C;
	int rv_C;

	memset(&hints_C, 0, sizeof hints_C);
	hints_C.ai_family = AF_UNSPEC;
	hints_C.ai_socktype = SOCK_DGRAM;

	if ((rv_C = getaddrinfo("127.0.0.1", C_PORT, &hints_C, &servinfo_C)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_C));
		return 1;
	}

	for(p_C = servinfo_C; p_C != NULL; p_C = p_C->ai_next) {
		if ((sockfd_C = socket(p_C->ai_family, p_C->ai_socktype,
				p_C->ai_protocol)) == -1) {
			perror("serverC: socket");
			continue;
		}
		break;
	}

	if (p_C == NULL)  {
		fprintf(stderr, "serverC: failed to bind\n");
		return 2;
	}

	//freeaddrinfo(servinfo_C); // all done with this structure




////////////////////////
	//printf("serverM: waiting for connections...\n");
	printf("The main server is up and running.\n");
////////////////////////


	while(1) {  // main accept() loop
		///////////////////////////////// client 1////////////////
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		//printf("server: got connection from %s\n", s);

		// receive from client
		char get_buf[MAXDATASIZE] = {0};
		strcpy(get_buf, recvFclient(new_fd));
		//printf("%s\n", get_buf);
		//printf("get from client: %s\n", get_buf);
		sscanf(get_buf, "%s%s%d", name1, name2, &amount);
		if (!strcmp(name2, "NULL")) {
			/////////////case1 search
			//printf("%s\n", "case1");
			printf("The main server received input=%s from the client using TCP over port 25965.\n", name1);

			// 计算钱，存储的money
			char balance[MAXBUFSIZE] = {0};
			strcpy(balance, get_balance(name1, sockfd_U, p_A, p_B, p_C)); // name1的总钱数
			//printf("balance is %s\n", balance);

			// 发送回client 或者 打印钱数
			//printf("%s\n", balance);
			send2client(new_fd, balance);
			printf("The main server sent the current balance to the client.\n");

		}
		else {
			///////////// case 2, transfer
			//printf("%s\n", "case2");
			printf("The main server received from %s to transfer %d coins to %s using TCP over port 25965.\n", name1, amount, name2);

			char balance1[MAXBUFSIZE] = {0};
			strcpy(balance1, get_balance(name1, sockfd_U, p_A, p_B, p_C)); // name1的总钱数
			char balance2[MAXBUFSIZE] = {0};
			strcpy(balance2, get_balance(name2, sockfd_U, p_A, p_B, p_C)); // name2的总钱数
			//printf("balance is %s and %s\n", balance1, balance2);

			char result[MAXBUFSIZE] = {0};

			if ((strcmp(balance1, "NULL") == 0) && (strcmp(balance2, "NULL") == 0)) {
				// both not exist, can't transfer
				strcpy(result, "NULL NULL");
			}
			else if((strcmp(balance1, "NULL") == 0) && (strcmp(balance2, "NULL") != 0)){
				// name1 not exist
				strcpy(result, "NULL ");
				strcat(result, name2);
			}
			else if((strcmp(balance1, "NULL") != 0) && (strcmp(balance2, "NULL") == 0)){
				// name2 not exist
				strcpy(result, name1);
				strcat(result, " NULL");
			}
			else {
				// both exist, transfer
				int temp_num = atoi(balance1);
				int cur_money = temp_num - amount;
				char str_curMoney[MAXBUFSIZE] = {0};
				itoa(cur_money, str_curMoney, 10);
				if (cur_money>=0) {
					// 钱够 可以转
					strcpy(result, name1);
					strcat(result, " ");
					strcat(result, name2);
					strcat(result, " ");
					strcat(result, str_curMoney);

					// 记录转账
					char transca[MAXBUFSIZE] = {0}; // 记录转账信息 transca =  get_buf + id
					int id = get_id(sockfd_U, p_A, p_B, p_C) + 1;
					//printf("current id is:%d\n", id);
					char str_id[MAXBUFSIZE] = {0};
					itoa(id, str_id, 10);
					strcpy(transca, get_buf);
					strcat(transca, " ");
					strcat(transca, str_id);
          //printf("transca:%s\n", transca);

					// random choose
					int a;
					srand((unsigned)time(NULL));
					a = rand() % 3 + 1; //1～3的随机数
					if(a==1){
						// store in A
						send2C(sockfd_U, transca, p_A, "A");
					}
					else if (a==2){
						// store in B
						send2C(sockfd_U, transca, p_B, "B");
					}
					else {
						//store in C
						send2C(sockfd_U, transca, p_C, "C");
					}
				}
				else {
					// 钱不够
					strcpy(result, "NOTENOUGH ");
					strcat(result, balance1);
				}

			}

			// 发送回client
			//printf("%s\n", result);
			send2client(new_fd, result);
			printf("The main server sent the result of the transaction to the client.\n");

		}





		///////////////////////////////// client 2 ////////////////
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		//printf("server: got connection from %s\n", s);

		// receive from client
		char get_buf2[MAXDATASIZE] = {0};
		strcpy(get_buf2, recvFclient(new_fd));
		//printf("%s\n", get_buf);
		//printf("get from client: %s\n", get_buf);
		sscanf(get_buf2, "%s%s%d", name1, name2, &amount);
		if (!strcmp(name2, "NULL")) {
			/////////////case1 search
			//printf("%s\n", "case1");
			printf("The main server received input=%s from the client using TCP over port 25965.\n", name1);

			// 计算钱，存储的money
			char balance[MAXBUFSIZE] = {0};
			strcpy(balance, get_balance(name1, sockfd_U, p_A, p_B, p_C)); // name1的总钱数
			//printf("balance is %s\n", balance);

			// 发送回client 或者 打印钱数
			//printf("%s\n", balance);
			send2client(new_fd, balance);
			printf("The main server sent the current balance to the client.\n");

		}
		else {
			///////////// case 2, transfer
			//printf("%s\n", "case2");
			printf("The main server received from %s to transfer %d coins to %s using TCP over port 25965.\n", name1, amount, name2);

			char balance1[MAXBUFSIZE] = {0};
			strcpy(balance1, get_balance(name1, sockfd_U, p_A, p_B, p_C)); // name1的总钱数
			char balance2[MAXBUFSIZE] = {0};
			strcpy(balance2, get_balance(name2, sockfd_U, p_A, p_B, p_C)); // name2的总钱数
			//printf("balance is %s and %s\n", balance1, balance2);

			char result[MAXBUFSIZE] = {0};

			if ((strcmp(balance1, "NULL") == 0) && (strcmp(balance2, "NULL") == 0)) {
				// both not exist, can't transfer
				strcpy(result, "NULL NULL");
			}
			else if((strcmp(balance1, "NULL") == 0) && (strcmp(balance2, "NULL") != 0)){
				// name1 not exist
				strcpy(result, "NULL ");
				strcat(result, name2);
			}
			else if((strcmp(balance1, "NULL") != 0) && (strcmp(balance2, "NULL") == 0)){
				// name2 not exist
				strcpy(result, name1);
				strcat(result, " NULL");
			}
			else {
				// both exist, transfer
				int temp_num = atoi(balance1);
				int cur_money = temp_num - amount;
				char str_curMoney[MAXBUFSIZE] = {0};
				itoa(cur_money, str_curMoney, 10);
				if (cur_money>=0) {
					// 钱够 可以转
					strcpy(result, name1);
					strcat(result, " ");
					strcat(result, name2);
					strcat(result, " ");
					strcat(result, str_curMoney);

					// 记录转账
					char transca2[MAXBUFSIZE] = {0}; // 记录转账信息 transca2 = get_buf + id
					int id = get_id(sockfd_U, p_A, p_B, p_C) + 1;
					//printf("current id is:%d\n", id);
					char str_id[MAXBUFSIZE] = {0};
					itoa(id, str_id, 10);
					strcpy(transca2, get_buf2);
					strcat(transca2, " ");
					strcat(transca2, str_id);
          //printf("transca2:%s\n", transca2);

					// random choose
					int a;
					srand((unsigned)time(NULL));
					a = rand() % 3 + 1; //1～3的随机数
					if(a==1){
						// store in A
						send2C(sockfd_U, transca2, p_A, "A");
					}
					else if (a==2){
						// store in B
						send2C(sockfd_U, transca2, p_B, "B");
					}
					else {
						//store in C
						send2C(sockfd_U, transca2, p_C, "C");
					}
				}
				else {
					// 钱不够
					strcpy(result, "NOTENOUGH ");
					strcat(result, balance1);
				}
			}

			// 发送回client
			//printf("%s\n", result);
			send2client(new_fd, result);
			printf("The main server sent the result of the transaction to the client.\n");
		}


		////////////////////////////////// monitor////////////////
		sin_size2 = sizeof their_addr2;
		new_fd2 = accept(sockfd2, (struct sockaddr *)&their_addr2, &sin_size2);
		if (new_fd2 == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr2.ss_family,
			get_in_addr((struct sockaddr *)&their_addr2),
			s2, sizeof s2);
		//printf("server: got connection from %s\n", s2);
		char get_buf3[MAXDATASIZE];
			if ((numbytes = recv(new_fd2, get_buf3, MAXDATASIZE-1, 0)) == -1) {
					perror("recv");
					exit(1);
			}
			//printf("%s\n", get_buf);

			if (!strcmp(get_buf3, "TXLIST")) {
				//print list
				//printf("%s\n", "print list");
				printf("The main server received a sorted list request from the monitor using TCP over port 26965.\n");
				// 问ABC要记录
				char info1[MAXDATASIZE] = {0}, info2[MAXDATASIZE] = {0}, info3[MAXDATASIZE] = {0};
				send2C(sockfd_U, get_buf3, p_A, "A");
				strcpy(info1, recvFC(sockfd_U, "A"));
				//printf("recv info from A:%s\n", info1);

				send2C(sockfd_U, get_buf3, p_B, "B");
				strcpy(info2, recvFC(sockfd_U, "B"));
				//printf("recv info from B:%s\n", info2);

				send2C(sockfd_U, get_buf3, p_C, "C");
				strcpy(info3, recvFC(sockfd_U, "C"));
				//printf("recv info from C:%s\n", info3);

				// 分割字符串，存储到struct
				int record_num = get_id(sockfd_U, p_A, p_B, p_C);
				//printf("%d\n", record_num);
				struct Transaction_2 T2[record_num];

				char records[MAXDATASIZE] = {0};
				strcpy(records, info1);
				strcat(records, info2);
				strcat(records, info3);
				//printf("records is:%s\n", records);
				const char sign[2] = " ";
			  char *token;
			  token = strtok(records, sign);
			  int k = 0;

			  while(token != NULL) {
			    switch(k%4) {
			      case 0:
			        strcpy(T2[k/4].id, token);
			        break;
			      case 1:
			        strcpy(T2[k/4].sender, token);
			        break;
			      case 2:
			        strcpy(T2[k/4].receiver, token);
			        break;
			      case 3:
			        strcpy(T2[k/4].amount, token);
			        break;
			    }
			    k++;
			    //printf("%s\n", token);

			    token = strtok(NULL, sign);
			  }

				//打印 检测T2

				for(int i=0; i<record_num; i++){
			    //sscanf(records, "%s%s%s%s\n", T2[i].id, T2[i].sender, T2[i].receiver, T2[i].amount);
			    //printf("%s %s %s %s\n", T2[i].id, T2[i].sender, T2[i].receiver, T2[i].amount);
			  }

				// 排序 + 打印输出到文件
				int q = 1; // 当前对象
			  char sorted_record[MAXDATASIZE] = {0};
			  while(q<record_num+1){
			    for(int p = 0; p<record_num+1; p++) {
			      int temp = atoi(T2[p].id);
			      //printf("%d\n", temp);
			      if (q == temp){
			        strcat(sorted_record, T2[p].id); strcat(sorted_record, " ");
			        strcat(sorted_record, T2[p].sender); strcat(sorted_record, " ");
			        strcat(sorted_record, T2[p].receiver); strcat(sorted_record, " ");
			        strcat(sorted_record, T2[p].amount); strcat(sorted_record, "\n");
			        q++;
			      }
			    }
			  }
			  //printf("\nsorted_record:\n%s\n", sorted_record);

				// 写入文件
				FILE *fp = NULL;
				fp = fopen(FILE_NAME, "w+");
				fprintf(fp, "%s", sorted_record);
				fclose(fp);

				/////////////////send to monitor
				if (send(new_fd2, "print success!", 13, 0) == -1)
					perror("send");
				close(new_fd2);

			}
			else {
				// error input
				printf("%s\n", "error input");
			}


	}




	return 0;
}
