// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch and the PBBS team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// {-r 1/2}

#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include "gettime.h"
#include "utils.h"
#include "graph.h"
#include "parallel.h"
#include <typeinfo>
#include "IO.h"
#include "graphIO.h"
#include "parseCommandLine.h"
#include "MIS.h"
#include <time.h>
#include <pthread.h>
using namespace std;
using namespace benchIO;
#include <stdbool.h>
#include <unistd.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <stdlib.h> 
#include <netinet/in.h> 
#include <time.h> 
#include <fcntl.h>
#include <poll.h>

// --> random max set as: 2147483647


#define PORT 8080 

#include <errno.h>  
#include <arpa/inet.h>  

#define NUM_THREADS     5

int primary = 0;
int reboot = 1;
int batchSize;
int checker = 0;
int CRC_val = 0;
char str[200];
int wait = 0;
const char* iFileSEND;
char* iFile;

pthread_t threads[NUM_THREADS];


uint32_t
rc_crc32(uint32_t crc, const char *buf, size_t len)
{
	static uint32_t table[256];
	static int have_table = 0;
	uint32_t rem;
	uint8_t octet;
	int i, j;
	const char *p, *q;
 
	/* This check is not thread safe; there is no mutex. */
	if (have_table == 0) {
		/* Calculate CRC table. */
		for (i = 0; i < 256; i++) {
			rem = i;  /* remainder from polynomial division */
			for (j = 0; j < 8; j++) {
				if (rem & 1) {
					rem >>= 1;
					rem ^= 0xedb88320;
				} else
					rem >>= 1;
			}
			table[i] = rem;
		}
		have_table = 1;
	}
 
	crc = ~crc;
	q = buf + len;
	for (p = buf; p < q; p++) {
		octet = *p;  /* Cast to unsigned octet. */
		crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
	}
	return ~crc;
}
 
int checksum(const char *number)
{
	const char *s = number;
	printf("%" PRIX32 "\n", rc_crc32(0, s, strlen(s)));
	CRC_val = rc_crc32(0, s, strlen(s));
	printf("VAL: %d", CRC_val);
	printf("\n ");
	return 0;
}

int genRandoms(int lower, int upper) { 
	int num = (rand() % (upper - lower + 1)) + lower; 
    return num; 
} 

void *primary_check(void *threadid) 
{
	printf("]n RUNNING PRIMARY CHECK \n");
	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	char buffer[1024] = {0}; 
	const char ack[12] = "acknowledge";
    const char fileRequest[12] = "needFileNow";
	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		printf("socket failed"); 
		return 0; 
	} 

	// STACK
	int status = fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL, 0) | O_NONBLOCK);

	if (status == -1){
		perror("calling fcntl");
		// handle the error.  By the way, I've never seen fcntl fail in this way
	}


	// Forcefully attaching socket to the port 8080 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
												&opt, sizeof(opt))) 
	{ 
		printf("setsockopt"); 
		return 0; 
	} 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( 8080 ); 
	
	// Forcefully attaching socket to the port 8080 
	if (bind(server_fd, (struct sockaddr *)&address, 
								sizeof(address))<0) 
	{ 
		printf("bind failed"); 
		return 0; 
	} 

	if (listen(server_fd, 3) < 0) 
	{ 
		printf("listen"); 
		return 0; 
	} 

    int counter = 0;
	while ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
					(socklen_t*)&addrlen))<0) 
	{ 
	  printf("\ntrying to accept number: ");
      printf("%d", counter); 
      counter++;
      usleep(500000);
      if (counter == 10){
         printf("\n CONNECTION TIMED OUT OF ACCEPT\n");
         return 0; 
      }
		
	}
	
	// GET MESSAGE

	while (counter < 10){
		valread = recv( new_socket , buffer, sizeof(buffer), 0); 
		if (valread <= 0){
			printf("\nread: ");
			printf("%d", valread);
			printf("\nbuffer is: ");
			printf(buffer);
			if (counter == 9){
				printf("\nConnection timed out");
				return 0;
			}

		} else {
			printf("\nJust read: ");
			printf("%d", valread);
			printf("\nbuffer is: ");
			printf(buffer);
			counter = 9;
		}
		printf("\nTrying to Read, Iteration #: ");
		printf("%d", counter);
		usleep(500000);
		counter++;
	}

	// SEND MESSAGE

	counter = 0;
	while (counter < 10){
		if (reboot == 1){
			valread = send(new_socket , ack , strlen(ack) , 0 );
		} else {
			valread = send(new_socket , fileRequest , strlen(fileRequest) , 0 );
		}
		if (valread <= 0){
			printf("\nsent: ");
			printf("%d", valread);
			if (counter == 9){
				printf("\nConnection timed out");
				return 0;
			}

		} else {
			printf("Just sent: ");
			printf("%d", valread);
			printf("\nmessage is: ");
			printf(ack);
			counter = 9;
		}
		printf("\nTrying to send, Iteration #: ");
		printf("%d", counter);
		usleep(500000);
		counter++;
	}
		
	printf("Acknowledge message sent\n"); 

	// if you need the iFile, GET FILE

	if (reboot != 1){

		counter = 0;
		while (counter < 10){
			valread = recv( new_socket , buffer, sizeof(buffer), 0); 
			if (valread <= 0){
				printf("\nread: ");
				printf("%d", valread);
				printf("\nbuffer is: ");
				printf(buffer);
				if (counter == 9){
					printf("\nConnection timed out");
					return 0;
				}

			} else {
				printf("\nJust read filename: ");
				printf("%d", valread);
				printf("\nFILENAME is: ");
				printf(buffer);
				iFile = buffer;
				counter = 9;
			}
			printf("\nTrying to Read, Iteration #: ");
			printf("%d", counter);
			usleep(500000);
			counter++;
		}

		reboot = 1;
	}

	close(new_socket);
	return 0;
} 


void *replica_check(void *threadid) 
{
	int s;
	int optval;
	socklen_t optlen = sizeof(optval);
	struct sockaddr_in serv_addr;
	int addrlen = sizeof(serv_addr);
	const char checkIn[] = "DID IT SEND?";
	//const char* fileName[1024] = {file};
	// strcpy(fileName, iFile);

	/* Create the socket */
	if((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket()");
		return 0;
	}

	// // STACK
	// int status = fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);

	// if (status == -1){
	// perror("calling fcntl");
	//    // handle the error.  By the way, I've never seen fcntl fail in this way
	// }

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(8080);  

	/* Check the status for the keepalive option */
	if(getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
		perror("getsockopt()");
		close(s);
		return 0;
	}
	printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));

	/* Set the option active */
	optval = 1;
	optlen = sizeof(optval);
	if(setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
		perror("setsockopt()");
		close(s);
		return 0;
	}
	printf("SO_KEEPALIVE set on socket\n");

	/* Check the status again */
	if(getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
		perror("getsockopt()");
		close(s);
		return 0;
	}
	printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));


	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\n Invalid address/ Address not supported \n"); 
		return 0; 
	} 

	int counter = 0;

	while (connect(s, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed Number: "); 
		printf("%d", counter);
		usleep(500000); 
		counter++;
		if (counter == 10){
			printf("\nCONNECTION TIMED OUT\n");
			return 0;
		}
	}

	printf("\n CONNECTION SUCCESS");

	counter = 0;
	// SEND MESSAGE
	while ((send(s , checkIn , strlen(checkIn) , 0) <= 0)){
		printf("\n SOMETHING WENT WRONG\n");
		usleep(500000);
		counter++;
		if (counter == 10){
			printf("\nCONNECTION TIMED OUT\n");
			return 0;
		}
	}

	int valread;
	counter = 0;
	char buffer[11];
	
	// GET MESSAGE

	while (counter < 10){
		valread = recv( s , buffer, sizeof(buffer), 0); 
		if (valread <= 0){
			printf("\nread: ");
			printf("%d", valread);
			printf("\nbuffer is: ");
			printf(buffer);
			if (counter == 9){
				printf("\nConnection timed out");
				return 0;
			}

		} else {
			printf("\n Just read: ");
			printf("%d", valread);
			printf("\nbuffer is: ");
			printf(buffer);
			if (strcmp(buffer, "acknowledge") == 0){
				printf("\nEVERYTHING IS OKAY\n");
			} else if (strcmp(buffer, "needFileNow") == 0){
				printf("\n NEED TO SEND FILE TO CRASHED SERVER\n");
				int secondCounter = 0;
				while ((send(s , iFile , strlen(iFile) , 0) <= 0)){
					printf("\n SOMETHING WENT WRONG\n");
					usleep(500000);
					secondCounter++;
					if (secondCounter == 10){
						printf("\nCONNECTION TIMED OUT\n");
						return 0;
					}
				}
				printf("sent FILENAME");
			}
			counter = 9;
		}
		printf("\nTrying to Read, Iteration #: ");
		printf("%d", counter);
		usleep(500000);
		counter++;
	}

	close(s);
	return 0;
} 


void *crash(void *threadid)
{
	usleep(5000);
    while (true){
        if(rand() <= 21474836470){
            printf("CRASH !!!!!!! CRASH !!!!!!! \n");
		    exit(EXIT_FAILURE);
		    exit(1);
        }
        usleep(5000);
	   //printf("\n CRASH! \n");
	   // exit(EXIT_FAILURE);
    }
    return 0;
}

void *masterNode(void *threadid)
{
   int rc;
   printf("Running master \n");
   long t = 2;
   while(true){
		if (primary == 1){
			rc = pthread_create(&threads[t], NULL, primary_check, (void *)t);
		}
		else {
			rc = pthread_create(&threads[t], NULL, replica_check, (void *)t);
		}
		usleep(50);
		printf("waiting to finish \n");
		pthread_join(threads[2], NULL);
		printf("\n finished \n");
		usleep(500);
		
   }
   return 0;
}

void timeMIS(graph<intT> G, int rounds, char* outFile) {
  graph<intT> H = G.copy(); //because MIS might modify graph
  char* flags = maximalIndependentSet(H);
  printf("\n HERE \n");
  printf("\n round is: ");
  printf("%d", rounds);
  srand(time(0));
  //char check[32];
  for (int i=0; i < rounds; i++) {	
	if (genRandoms(1, 1000000) < 100){
		return;
	}
	for (int x =0; x < sizeof(flags); x++){
		//char add[6];
		//printf("flags \n");
		//cout << int(flags[x]);
		//printf("checker: \n");
		checker = checker*10 + int(flags[x]);
		//cout << checker;
		//sprintf(add, "%f", int(flags[x]));
		//strcat(check, add);
	}
	printf("\n this was flags \n");
	char buf[32];
	sprintf(buf, "%d", checker);
	const char *p = buf;
	printf("\n this is p: ");
	cout << p;
	checksum(p);
	printf(typeid(flags).name());
    free(flags);
    H.del();
    H = G.copy();
    startTime();	
    flags = maximalIndependentSet(H);
    nextTimeN();
  }
  cout << endl;
  printf("\n now printing outFile \n");
  if (outFile != NULL) {
    int* F = newA(int, G.n);
    for (int i=0; i < G.n; i++) 
    {
      F[i] = flags[i];
    }
    writeIntArrayToFile(F, G.n, outFile);
    free(F);
  }
  
  free(flags);
  G.del();
  H.del();
}


int runAsPrimary(char* name){
  printf("\n running server as primary \n");
	int sock = 0, valread; 
	struct sockaddr_in serv_addr; 
	char *fileName = name; 
	char buffer[1024] = {0}; 
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(PORT); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\n Invalid address/ Address not supported \n"); 
		return -1; 
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n"); 
		return -1; 
	} 
	send(sock , fileName , strlen(fileName) , 0 ); 
	printf("\n Filename message sent\n"); 
	valread = recv( sock , buffer, sizeof(buffer), 0); 
	printf("%s\n",buffer ); 
	return 0;
}

void *checkCRCprimary(void *threadid)
{
	sprintf(str,"%d", CRC_val);
	printf("\n");
	printf(str);
	printf("\n");
  	printf("\n Checker running as primary \n");
	int s;
	int optval;
	socklen_t optlen = sizeof(optval);
	struct sockaddr_in serv_addr;
	int addrlen = sizeof(serv_addr);
	char* crc_checker = str;
	

	/* Create the socket */
	if((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket()");
		return 0;
	}

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(5080);  

	/* Check the status for the keepalive option */
	if(getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
		perror("getsockopt()");
		close(s);
		return 0;
	}
	printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));

	/* Set the option active */
	optval = 1;
	optlen = sizeof(optval);
	if(setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
		perror("setsockopt()");
		close(s);
		return 0;
	}
	printf("SO_KEEPALIVE set on socket\n");

	/* Check the status again */
	if(getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
		perror("getsockopt()");
		close(s);
		return 0;
	}
	printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));


	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\n Invalid address/ Address not supported \n"); 
		return 0; 
	} 

	int counter = 0;

	while (connect(s, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed Number: "); 
		printf("%d", counter);
		usleep(500000); 
		counter++;
		if (counter == 10){
			printf("\nCONNECTION TIMED OUT\n");
			return 0;
		}
	}

	printf("\n CONNECTION SUCCESS");

	counter = 0;
	// SEND MESSAGE
	while ((send(s , crc_checker , strlen(crc_checker) , 0) <= 0)){
		printf("\n SOMETHING WENT WRONG\n");
		usleep(500000);
		counter++;
		if (counter == 10){
			printf("\nCONNECTION TIMED OUT\n");
			return 0;
		}
	}

	printf("\n JUST SENT CRC VAL \n");

	int valread;
	counter = 0;
	char buffer[1024];
	
	// GET MESSAGE

	while (counter < 10){
		valread = recv( s , buffer, sizeof(buffer), 0); 
		if (valread <= 0){
			printf("\nread: ");
			printf("%d", valread);
			printf("\nbuffer is: ");
			printf(buffer);
			if (counter == 9){
				printf("\nConnection timed out");
				return 0;
			}

		} else {
			printf("\n Just read CRC CHECK: ");
			printf("%d", valread);
			printf("\nCRC CHECK WAS: ");
			printf(buffer);
			counter = 9;
		}
		printf("\nTrying to CHECK CRC, Iteration #: ");
		printf("%d", counter);
		usleep(500000);
		counter++;
	}

	close(s);
	return 0;
}


int runAsReplica(){
  	printf("\n running as replica \n");
  	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	char buffer[1024] = {0}; 
	string ackstr = "acknowledge";
	char *ack = (char*) ackstr.c_str();
	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		printf("socket failed"); 
		return 1; 
	} 
	// Forcefully attaching socket to the port 8080 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
												&opt, sizeof(opt))) 
	{ 
		printf("setsockopt"); 
		return 1; 
	} 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( PORT ); 

	// Forcefully attaching socket to the port 8080 
	if (bind(server_fd, (struct sockaddr *)&address, 
								sizeof(address))<0) 
	{ 
		printf("bind failed"); 
		return 1; 
	} 
	if (listen(server_fd, 3) < 0) 
	{ 
		printf("listen"); 
		return 1; 
	} 
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
					(socklen_t*)&addrlen))<0) 
	{ 
		printf("accept"); 
		return 1; 
	} 
	valread = recv( new_socket , buffer, sizeof(buffer), 0); 
	iFile = buffer;
	printf("%s\n",buffer ); 
	send(new_socket , ack , strlen(ack) , 0 ); 
	printf("Acknowledge message sent\n"); 
	close(new_socket);
	return 0;
}


void *checkCRCreplica(void *threadid)
{
  	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	char buffer[1024] = {0}; 
	char *ack = (char*) string("NOT MATCHING").c_str(); 
	char *confirm = (char*) string("CONFIRMED").c_str();
	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		printf("socket failed"); 
		return 0; 
	} 

	// STACK
	int status = fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL, 0) | O_NONBLOCK);

	if (status == -1){
		perror("calling fcntl");
		// handle the error.  By the way, I've never seen fcntl fail in this way
	}


	// Forcefully attaching socket to the port 8080 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
												&opt, sizeof(opt))) 
	{ 
		printf("setsockopt"); 
		return 0; 
	} 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( 5080 ); 
	// Forcefully attaching socket to the port 5080 
	if (bind(server_fd, (struct sockaddr *)&address, 
								sizeof(address))<0) 
	{ 
		printf("bind failed"); 
		return 0; 
	} 

	if (listen(server_fd, 3) < 0) 
	{ 
		printf("listen"); 
		return 0; 
	} 

    int counter = 0;
	while ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
					(socklen_t*)&addrlen))<0) 
	{ 
	  printf("\ntrying to accept number: ");
      printf("%d", counter); 
      counter++;
      usleep(500000);
      if (counter == 10){
         printf("\n CONNECTION TIMED OUT OF ACCEPT\n");
         return 0; 
      }
		
	} 
	
	// GET MESSAGE
	bool valsAreTheSame = false;

	while (counter < 10){
		valread = recv( new_socket , buffer, sizeof(buffer), 0); 
		if (valread <= 0){
			printf("\nread: ");
			printf("%d", valread);
			printf("\nbuffer is: ");
			printf(buffer);
			if (counter == 9){
				printf("\nConnection timed out");
				return 0;
			}

		} else {
			printf("\nJust read CRC VAL: ");
			printf("%d", valread);
			printf("\nCRC VAL is: ");
			printf(buffer);

			int chec = atoi(buffer);
			chec == int(CRC_val); 
			if (chec == int(CRC_val)){
				valsAreTheSame = true;
				printf("\nCRC VALS ARE THE SAME\n"); 
			} else {
				valsAreTheSame = false;
				printf("\nCRC VALS ARE DIFFERENT\n"); 
			}

			counter = 9;
		}
		printf("\nTrying to Read, Iteration #: ");
		printf("%d", counter);
		usleep(500000);
		counter++;
	}

	// SEND MESSAGE

	counter = 0;
	while (counter < 10){
		if (valsAreTheSame){
			valread = send(new_socket , confirm , strlen(confirm) , 0 );
		} else {
			valread = send(new_socket , ack , strlen(ack) , 0 );
		}
		if (valread <= 0){
			printf("\nsent: ");
			printf("%d", valread);
			if (counter == 9){
				printf("\nConnection timed out");
				return 0;
			}

		} else {
			printf("Just sent CRC MESSAGE: ");
			printf("%d", valread);
			printf("\nmessage is: ");
			printf(ack);
			counter = 9;
		}
		printf("\nTrying to send, Iteration #: ");
		printf("%d", counter);
		usleep(500000);
		counter++;
	}

	close(new_socket);
	return 0;
	
}


int sendToClient(int arg, int CRC_val){
	#undef PORT
	#define PORT 3490
    printf("\n sending to client \n");
    int sock = 0, valread; 
	struct sockaddr_in serv_addr; 

    char name[300]; 
	sprintf(name, "%d", arg);
	char *fileName = name; 

	char nameTwo[300]; 
	sprintf(nameTwo, "%d", CRC_val);
	char *fileNameTwo = nameTwo; 
	
	char buffer[1024] = {0}; 
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(PORT); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\n Invalid address/ Address not supported \n"); 
		return -1; 
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed With Client\n"); 
		printf(strerror(errno));
		return -1; 
	} 
	send(sock , fileName , strlen(fileName) , 0 ); 
	usleep(1000);
	send(sock , fileNameTwo , strlen(fileNameTwo) , 0 ); 
	printf("\n Filename message sent TO CLIENT\n"); 
	return 0;
}

int parallel_main(int argc, char* argv[]) {
  setbuf(stdout, NULL);
  commandLine P(argc, argv, "[-o <outFile>] [-p <primary>] [-t <reboot>] [-r <rounds>] [-b <batchSize>] [-c <threadCount>] <inFile>");
  printf("\n started main \n");
  char* iFile = P.getArgument(0);
  iFileSEND = iFile;
  char* oFile = P.getOptionValue("-o");
  int rounds = P.getOptionIntValue("-r",1);
  primary = P.getOptionIntValue("-p", 1);
  reboot = P.getOptionIntValue("-t", 1);
  batchSize = P.getOptionIntValue("-b", 10000);
  int cilkThreadCount = P.getOptionIntValue("-c", -1);
  int rc;
  long t;
  printf("about to create threads \n");
  if(primary == 1 && reboot != 2){
	printf("\n reboot is: ");
  	printf("%d", reboot);
    runAsPrimary(iFile);
  }
//   } else if (primary == 1 && reboot == 2){
// 		t = 0;
//     	rc = pthread_create(&threads[t], NULL, masterNode, (void *)t);
//   }
  else if(primary != 1) {
    runAsReplica();
	t = 0;
	rc = pthread_create(&threads[t], NULL, masterNode, (void *)t);
  }
  else if(primary == 1 && reboot == 2){
	  t = 0;
	  rc = pthread_create(&threads[t], NULL, primary_check, (void *)t);
	  pthread_join(threads[0], NULL);
	  rc = pthread_create(&threads[t], NULL, masterNode, (void *)t);
  }
  if (primary == 1 && reboot != 2){
	  t = 1;
	  // usleep(50000);
  	  rc = pthread_create(&threads[t], NULL, crash, (void *)t);
  }

  if(cilkThreadCount > 0)
	{
		//std::string s = std::to_string(cilkThreadCount);
		char num[3];
		sprintf(num,"%d",cilkThreadCount);
		__cilkrts_end_cilk();
		__cilkrts_set_param("nworkers", num);
		__cilkrts_init();
		std::cout << "The number of threads " << cilkThreadCount << std::endl;
	}
  graph<intT> G = readGraphFromFile<intT>(iFile);
  timeMIS(G, rounds, oFile);
  sprintf(str,"%d", CRC_val);
  char *message = str;
  sendToClient(checker, CRC_val);
  if (primary != 1){
	  t = 3;
	  //wait = 1;
  	  rc = pthread_create(&threads[t], NULL, checkCRCreplica, (void *)t);
	  pthread_join(threads[3], NULL);
  }
  if (primary == 1){
	  t = 3;
	  //wait = 1;
	  usleep(1000);
  	  rc = pthread_create(&threads[t], NULL, checkCRCprimary, (void *)t);
	  pthread_join(threads[3], NULL);
  }
  printf("check:  ");
  cout << checker;
  cout << CRC_val;
  pthread_join(threads[3], NULL);
  printf("DONE \n");
  printf("EXITING \n");
  exit(0);
}