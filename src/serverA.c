/*
  basecode from https://beej.us/guide/bgnet/examples/listener.c
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

#define A_PORT "21965"  // the port users will be connecting to

#define FILE_NAME "block1.txt"
#define ROWSIZE 50000
#define BUFSIZE 10000
#define MAXBUFLEN 10000


char str2[BUFSIZE] = {0};
char bla[BUFSIZE] = {0};
char enc[BUFSIZE] = {0};

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

struct Transaction_A{
  int id;
  char sender[BUFSIZE];
  char receiver[BUFSIZE];
  int amount;
};

int calculate_row(){
  FILE *fp = NULL;
  fp = fopen(FILE_NAME, "r");
  if (fp == NULL) {
    fprintf(stderr, "fail load file.\n");
    exit(EXIT_FAILURE);
  }

  // 计算行数
  char c[1000];
  int T_num = 0; // 行数/交易数
  while(!feof(fp)){
    fgets(c, 1000, fp);
    T_num++;
  }
  //printf("%d\n", T_num); //打印行数

  fclose(fp);

  return T_num;
}

char * encode_s(char* str){
  int len = strlen(str);
  memset(enc, 0x00, sizeof(char) * BUFSIZE); // set str2 to NULL
  for (int i=0; i<len; i++) {
    if((str[i] < 91) && (str[i] >= 88)){
      //XYZ
      str[i] -= 26;
    }
    else if((str[i] < 123) && (str[i] >= 120)){
      //xyz
      str[i] -= 26;
    }
    else if((str[i] < 58) && (str[i] >= 55)){
      //789
      str[i] -= 10;
    }

    enc[i] = str[i] + 3;
  }
  //printf("%s\n", str2);


  return enc;
}

char * decode_S(char* str1){
  int len = strlen(str1);
  //printf("%s\n", str1);
  //printf("len is:%d\n", len);
  memset(str2, 0x00, sizeof(char) * BUFSIZE); // set str2 to NULL
  for (int i=0; i<len; i++) {
    if((str1[i] < 68) && (str1[i] >= 65)){
      //ABC
      str1[i] += 26;
    }
    else if((str1[i] < 100) && (str1[i] >= 97)){
      //abc
      str1[i] += 26;
    }
    str2[i] = str1[i] - 3;
  }
  //printf("%s\n", str2);


  return str2;
}

int decode_n(char* str1){
  int num2 = 0;
  //printf("str1:%s\n", str1);

  int len = strlen(str1);
  char str2[BUFSIZE] = {0};
  //memset(str2, 0x00, sizeof(char) * 20); // set str2 to NULL
  for (int i=0; i<len; i++) {
    if((str1[i]<51) && (str1[i]>=48)){
      str1[i] += 10;
    }
    str2[i] = str1[i] - 3;
  }
  //printf("str2:%s\n", str2);

  num2 = atoi(str2);

  return num2;
}

char * cal_balance(int row_num, char* buf, struct Transaction_A *T){
 memset(bla, 0x00, sizeof(char) * BUFSIZE); // set bla to NULL
	int money = 0;
	int count_num = 0;

	for(int i=0; i<row_num-1; i++){
		if (!strcmp(buf, T[i].sender)){
			money -= T[i].amount;
			count_num++;
		}
		else if(!strcmp(buf, T[i].receiver)){
			money += T[i].amount;
			count_num++;
		}
	}

	if (count_num == 0){
		// 查无此人
		//printf("not exist:%s\n", "NULL");
		strcpy(bla, "NULL");
	}
	else{
		// 返回钱数
		//printf("money:%d\n", money);
		itoa(money, bla, 10); // 转化为str
		//printf("money char:%s\n", bla);
	}
	return bla;
}

void send2M (int sockfd, char * balance, struct sockaddr_storage their_addr,
	socklen_t sin_size, int numbytes){
	if ((numbytes=sendto(sockfd, balance, strlen(balance), 0,
		(struct sockaddr *)&their_addr, sin_size)) == -1) {
		 perror("sendto");
		 exit(1);
	 }
	 printf("The ServerA finished sending the response to the Main Server.\n");
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
  char buf[MAXBUFLEN] = {0};
  int numbytes;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, A_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("serverA: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("serverA: bind");
			continue;
		}

		break;
	}

  if (p == NULL)  {
    fprintf(stderr, "serverA: failed to bind\n");
    return 2;
  }

	freeaddrinfo(servinfo); // all done with this structure



	//printf("serverC: waiting for connections...\n");
	printf("The ServerA is up and running using UDP on port 21965.\n");



	while(1) {  // main accept() loop
    sin_size = sizeof their_addr;

    memset(buf, 0x00, sizeof(char) * MAXBUFLEN);
  	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
  		(struct sockaddr *)&their_addr, &sin_size)) == -1) {
  		perror("recvfrom");
  		exit(1);
  	}
		printf("The ServerA received a request from the Main Server.\n");

		///////////////////////////
	  // 计算行数
	  int row_num = calculate_row();

	  // 生成struct
	  struct Transaction_A T[row_num-1];

	  // 从txt中读取交易数据，存入struct
	  FILE *fp = NULL;
	  fp = fopen(FILE_NAME, "r");
	  if (fp == NULL) {
	    fprintf(stderr, "fail load file.\n");
	    exit(EXIT_FAILURE);
	  }
	  char row[ROWSIZE];
	  int i = 0;
	  char temp_send[BUFSIZE];
	  char temp_recv[BUFSIZE];
	  char temp_amount[BUFSIZE];
	  while (fgets(row, ROWSIZE, fp) != NULL) {
	    //sscanf(row, "%d%s%s%d", &T[i].id, T[i].sender, T[i].receiver, &T[i].amount);
	    sscanf(row, "%d%s%s%s", &T[i].id, temp_send, temp_recv, temp_amount);
	    //printf("%d %s %s %d\n", T[i].id, temp_send, temp_recv, temp_amount);
	    strcpy(T[i].sender, decode_S(temp_send));
	    strcpy(T[i].receiver, decode_S(temp_recv));
	    T[i].amount = decode_n(temp_amount);
	    //printf("%d %s %s %d\n", T[i].id, T[i].sender, T[i].receiver, T[i].amount); //验证
	    i++;
	  }
	  fclose(fp);
	  ///////////////////////////


		/////////////////////////////
		// 比较名字，回复钱数
    //printf("%s\n", buf); // ok

    char name1[MAXBUFLEN] = {0}, name2[MAXBUFLEN] = {0};
    char amount[MAXBUFLEN] = {0}, id[MAXBUFLEN] = {0};
		sscanf(buf, "%s%s%s%s", name1, name2, amount, id);
    //printf("name1 is %s,%s,%s,%s\n", name1, name2, amount, id);

		if(!strcmp(name2, "")){
			if(!strcmp(name1, "ID")) {
			 // get max id, return current column number
			 int cur_row = calculate_row() - 1;
       //printf("max id is %d\n", max_id);
			 char str_id[MAXBUFLEN];
			 itoa(cur_row, str_id, 10);
			 send2M(sockfd, str_id, their_addr, sin_size, numbytes); //send id toM

		 }
		 else if(!strcmp(name1, "TXLIST")){
			 // print all
       //printf("TX:%s", name1);

       // 生成记录string
       char records[ROWSIZE] = {0};
			 row_num = calculate_row();
			 for(int i=0; i<row_num-1; i++) {
         char temp_id[MAXBUFLEN] = {0}, temp_amt[MAXBUFLEN] = {0};
         itoa(T[i].id, temp_id, 10);
         itoa(T[i].amount, temp_amt, 10);
         strcat(records, temp_id); strcat(records, " ");
         strcat(records, T[i].sender); strcat(records, " ");
         strcat(records, T[i].receiver); strcat(records, " ");
         strcat(records, temp_amt); strcat(records, " ");
       }
       //printf("%s\n", records);

       // 传给serverM
       send2M(sockfd, records, their_addr, sin_size, numbytes);


		 }
		 else{
			 // get balance
			 //printf("get balance");

			 char balance[MAXBUFLEN] = {0};
			 row_num = calculate_row();
			 strcpy(balance, cal_balance(row_num, buf, T));
			 //printf("money balance at main:%s\n", balance);

			 send2M(sockfd, balance, their_addr, sin_size, numbytes); // send balance
		 }
		}
		else{
      //transfer, print to file
      char str_trans[MAXBUFLEN] = {0};
      char enc1[MAXBUFLEN] = {0}, enc2[MAXBUFLEN] = {0}, enc3[MAXBUFLEN] = {0};

      strcpy(enc1, encode_s(name1));
      strcpy(enc2, encode_s(name2));
      strcpy(enc3, encode_s(amount));

      // str_trans = id + name1 + name2 + amount
      strcpy(str_trans, id); strcat(str_trans, " ");
      strcat(str_trans, enc1); strcat(str_trans, " ");
      strcat(str_trans, enc2); strcat(str_trans, " ");
      strcat(str_trans, enc3);

      FILE *fp = NULL;
      fp = fopen(FILE_NAME, "a+");
      fprintf(fp, "%s\n", str_trans);
      fclose(fp);
    }


		/////////////////////////////


  	//close(sockfd);

  }
  return 0;
}
