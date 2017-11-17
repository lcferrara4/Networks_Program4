/* Networks Program 4
 * Lauren Ferrara - lferrara
 * Charlie Newell - cnewell1
 *
 * Server side of Chat Service
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h> // for threading, link with lpthread

#define BUFSIZE 4096
#define MAX_PENDING 5

// threading function
void *connection_handler(void *);

int receiveInt(int bits, int socket) {
	int size = 0;
	if (bits == 32) {
		int32_t ret;
		char *data = (char*)&ret;
		int int_size = sizeof(ret);
		if (recv(socket, data, int_size, 0) < 0) {
			perror("FTP Server: Error receiving file/directory size\n");
			exit(1);
		}
		size = ntohl(ret);
	} else if (bits == 16) {
		int16_t ret;
		char *data = (char*)&ret;
		int int_size = sizeof(ret);
		if (recv(socket, data, int_size, 0) < 0) {
			perror("FTP Server: Error receiving file/directory size\n");
			exit(1);
		}
		size = ntohs(ret);
	}
	return size;
}

void sendInt(int x, int bits, int socket) {
	if (bits == 32) {
		int32_t conv = htonl(x);
		char *data = (char*)&conv;
		int size_int = sizeof(conv);
		// Write size of listing as 32-bit int		
	        printf("%i\n", x);	
		if (write(socket, data, size_int) < 0) {
			perror("FTP Server: Error sending size of directory listing\n");
			exit(1);
		}

	} else if (bits == 16) {
		int16_t conv = htons(x);
		char *data = (char*)&conv;
		int size_int = sizeof(conv);
		// Write size of listing as 16-bit int		
		if (write(socket, data, size_int) < 0) {
			perror("FTP Server: Error sending size of directory listing\n");
			exit(1);
		}

	}	
}

void listDirectory(int socket, char *listing){
	int new_len;
	FILE *fp = popen("ls -l", "r");
	if(fp != NULL){
        	new_len = fread(listing, sizeof(char), BUFSIZE, fp);
		if(ferror(fp) != 0){
			perror("Error reading file\n");
			exit(1);
        	}
        	pclose(fp);
           	listing[new_len++] = '\0';
    	}
	sendInt(new_len, 32, socket);
	
	// Write listing
	if (write(socket, listing, new_len) < 0) {
		perror("FTP Server: Error listing directory contents\n");
		exit(1);
	}
}

int getFileSize(char* filename){
	int file_size;
	FILE *fp = fopen(filename, "r");
	fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	return file_size;
}

void getFileDir(int socket, char *filename) {
	int name_size = receiveInt(16, socket);
	// Get filename/directory name from client
	if (recv(socket, filename, name_size, 0) == -1 ) {
		perror("FTP Server: Unable to receive filename\n");
		exit(1);
	}
}

int main (int argc, char *argv[]) {

	//variable declaration
	int port, s, len,  s_new;
	int c, client_sock, 
	int waiting = 1;
	struct sockaddr_in server, client;	
	int opt = 1; /* 0 to disable options */
	char buffer[BUFSIZE], inner_buffer[BUFSIZE], listing[BUFSIZE], filename[BUFSIZE];
	struct stat st = {0};

	//check command line arguments
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [Port]\n", argv[0]);
		exit(1);
	}
	port = atoi(argv[1]);

	//build address data structure
	bzero((char *)&server, sizeof(server));

	//setup socket
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		perror("Chatserver: Unable to open socket\n");
		exit(1);
	}
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	//set socket option to allow reuse of port
	if ((setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(int))) < 0 ) {
		perror("Chatserver: Unable to set socket options\n");
		exit(1);
	}

	//bind socket
	if ((bind(s, (struct sockaddr*)&server, sizeof(server))) < 0 ) {
		perror("Chatserver: Unable to bind to socket\n");
		exit(1);
	}

	//listen for connection
	if ((listen(s, MAX_PENDING)) < 0 ) {
		perror("Chatserver: Unable to listen\n");
		exit(1);
	}

	// Accept incoming connection
	c = sizeof(struct sockaddr_in);
	pthread_t thread_id;

	// TODO: Set max number of threads 
	while( (client_sock = accept(s, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
	
		if (pthread_create( &thread_id, NULL, connection_handler, (void*) &client_sock) < 0 ) {
			perror("Chatserver: Could not create thread\n");
			exit(1);
		}
	}

	if (client_sock < 0) {
		perror("Chatserver: Unable to accept connection\n");
		exit(1);
	}
	
	return 0;
}

// Handles connection for each client
void *connection_handler(void *socket_desc) {

	// Get socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;
	char *message, client_message[2000];

	// Check whether user exists (store in file not memory): TODO
	message = "Welcome back. Enter your password >> \r";
	message = "New user? Create password >> \r";
	write(sock, message, strlen(message));
	
	// Save user and password (multiple clients should be able to register at once??): TODO
	
	// Wait for operation
	while ( (read_size = recv(sock, client_message, 2000, 0)) > 0) {	

		// end of string marker
		client_message[read_size] = '\0';

		if (!strcmp(client_message, "E")) {
			// Exit
			close(sock);
			// Update record of socket descriptors and usernames of active clients: TODO
			// join ??
			printf("Chatserver: Client has closed connection.\n");
		} else if (!strcmp(client_message, "B")) {
			// Message Broadcasting
			//bzero((char*)&listing, sizeof(listing));
			//listDirectory(s_new, listing);
		} else if (!strcmp(client_message, "P")) {
			// Private Messaging
		} else {
			printf("That operation doesn't exist. Please try again.");
		}

	}
	exit(0);
}

