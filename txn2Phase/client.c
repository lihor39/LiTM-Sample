#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <features.h>
#include <arpa/inet.h>

#define PORT 3490

int runAsClient(){
    printf("\n running as Client \n");
    int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	char buffer[1024] = {0}; 
	
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
	valread = read( new_socket , buffer, 1024); 
	printf("%s\n",buffer ); 
	valread = read( new_socket , buffer, 1024); 
	printf("%s\n",buffer ); 
	printf("\n CLIENT Recieved Signal\n"); 
	return 0;
}

int sendToClient(int arg, int CRC_val){
    printf("\n sending to client \n");
	#undef PORT
	#define PORT 3490
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
		printf("\nConnection Failed \n"); 
		return -1; 
	} 
	send(sock , fileName , strlen(fileName) , 0 ); 
	usleep(1000);
	send(sock , fileNameTwo , strlen(fileNameTwo) , 0 ); 
	printf("\n Filename message sent\n"); 
	return 0;
}

int main() {
    runAsClient();
    return 0;
}