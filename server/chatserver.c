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
#define MAX_USERS 10


// threading function
void *connection_handler(void *);

//global struct used to keep track user and its socket descriptors
struct activeUser {
	char* user_name;
	int sd;
};

//Array that stores all the structs
struct activeUser allUsers[MAX_USERS];

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

int checkUser(char* user_name) {
	FILE* fstream = fopen("users.txt", "r");
	char line[1024];
	char *parse;
	char user_pass[2][1024];


	while(fgets(line, 1024, fstream)) {
		parse = strtok(line, ",");
		strcpy(user_pass[0], parse);
		parse = strtok(NULL, ",");
		strcpy(user_pass[1], parse);
		
		if (strcmp(user_pass[0], user_name) == 0){
			return 1;
	
		}
	}	
	return 0;

       
}

int checkNamePass(char* user_name, char* pass) {
	FILE* fstream = fopen("users.txt", "r");
	char line[1024];
	char *parse;
	char user_pass[2][1024];

	//read in list of users and passwords
	while(fgets(line, 1024, fstream)) {
				
		parse = strtok(line, ",");
		strcpy(user_pass[0], parse);
		parse = strtok(NULL, "\n");
		strcpy(user_pass[1], parse);
	
		//compare user_name and password and return one if they match
		if ((strcmp(user_pass[0], user_name) == 0) && (strcmp(user_pass[1], pass) == 0)) {
			return 1;
		}
	}
	return 0;
}

void writeUserToFile(char* user_name, char* pass) {
	FILE* fstream = fopen("users.txt", "a");
	fprintf(fstream, "%s,%s\n", user_name, pass);
}

int addActiveUser(char* username, int sock) {
	int i;
	for (i = 0; i < MAX_USERS; i++) {
		if (allUsers[i].user_name == NULL) {
			allUsers[i].user_name = username;
			allUsers[i].sd = sock;
			return 1;
		}
	}
	return 0;  
}

int userOnline(char* username){
	int i;
        for (i = 0; i < MAX_USERS; i++) {
                if (allUsers[i].user_name != NULL && !strcmp(allUsers[i].user_name, username)) {
                        return i;
                }
        }
	return -1;
}

void printUsers() {
	int i;
	for (i = 0; i < MAX_USERS; i++) {
		if(allUsers[i].user_name != NULL) {
			printf("%s\n", allUsers[i].user_name);
		}
	}
}

char* formatMessage(char* message, char* type) {

	
	int newsize = strlen(message) + 1;
	char * newBuffer = (char *)malloc(newsize);

	strcpy(newBuffer, type);
	strcat(newBuffer, message);

	return newBuffer;
}

int main (int argc, char *argv[]) {

	//variable declaration
	int port, s, len,  s_new;
	int c, client_sock; 
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

	//initialize user array to null
	int i;
	for (i = 0; i < MAX_USERS; i++) {
		allUsers[i].user_name = NULL;
		allUsers[i].sd = -1;
	}

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
	
	pthread_join( thread_id , NULL);
	
	return 0;
}

// Handles connection for each client
void *connection_handler(void *socket_desc) {

	// Get socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;
	char user_name[100];
	int validPass = 0;
	int validAdd;
	int new_user;
	int i;
	int userNamePass;
	char *message, *response_message, client_message[2000];
	char client_input[10000];
	char *data_type;
	
	//Receive client_message which is user name
	read_size = receiveInt(16, sock);
	read(sock, client_message, read_size);

	//save user name in user_name variable
	strcpy(user_name, client_message);

	//check if this user exists
        new_user = checkUser(client_message);

	//send proper message depending on whether use exists or not
	if (new_user) {
		message = "Welcome back. Enter your password >> ";
	} else {
		message = "New user? Create password >> ";
	}
	message = formatMessage(message, "C");

	//send proper message back to client
	sendInt(strlen(message), 32, sock);
	write(sock, message, strlen(message));

	//zero out client message
	bzero((char *) client_message, sizeof(client_message));
	
	//receive password from client
	read_size = receiveInt(16, sock);
	read(sock, client_message, read_size);

	//check if user name and password match
	if (new_user) {
		userNamePass = checkNamePass(user_name, client_message);
		if(!userNamePass) {
			response_message = "Wrong password! Exiting!\n";
			response_message = formatMessage(response_message, "C");
			
			//send proper response back to client	
			sendInt(strlen(response_message), 32, sock);
			write(sock, response_message, strlen(response_message));

			return;
       		} else {
			response_message = "Welcome back!\n";
		}
	} else {
		writeUserToFile(user_name, client_message);
		response_message = "Welcome new user!\n";
	}
	
	response_message = formatMessage(response_message, "C");
	
	//send proper response back to client	
	sendInt(strlen(response_message), 32, sock);
	write(sock, response_message, strlen(response_message));

	//add user info to array that stores all users info
	validAdd = addActiveUser(user_name, sock);

	//zero out client message
	bzero((char *) client_message, sizeof(client_message));

	// Wait for operation
	while ( (read_size = recv(sock, client_message, 2000, 0)) > 0) {	

		// end of string marker
		client_message[read_size] = '\0';

		if (!strcmp(client_message, "E")) {

			//send confirmation that server connection has closed
			message = "Connection to server closed.\n";
			message = formatMessage(message, "C");

			//send proper message back to client
			sendInt(strlen(message), 32, sock);
			write(sock, message, strlen(message));

			close(sock);

			//reset values in allUsers array so spot can be used again
			for(i = 0; i < MAX_USERS; i++) {
				if(allUsers[i].sd == sock) {
					allUsers[i].user_name = NULL;
					allUsers[i].sd = -1;
				}
			}
			printf("Chatserver: Client has closed connection.\n");
			break;
		
		} else if (!strcmp(client_message, "B")) {
			// Message Broadcasting

			read_size = receiveInt(32, sock);
			read(sock, message, read_size);
			
			for(i = 0; i < MAX_USERS; i++) {
				if(allUsers[i].user_name != NULL && allUsers[i].sd != sock) {
					message = formatMessage(message, "D");
		
					sendInt(strlen(message), 32, allUsers[i].sd);	
					write(allUsers[i].sd, message, strlen(message));
				}
			}

			// Send acknowledgment
			response_message = formatMessage(response_message, "C");
			sendInt(strlen(response_message), 32, sock);
			write(sock, response_message, strlen(response_message));
				

		} else if (!strcmp(client_message, "P")) {
			// Private Messaging
				
			char* buffer = "C";
			int newsize = 0;
			for(i = 0; i < MAX_USERS; i++) {
				if(allUsers[i].user_name != NULL) {
					newsize = strlen(buffer) + strlen(allUsers[i].user_name) + 2;
					char * newBuffer = (char *)malloc(newsize);
	
					strcpy(newBuffer, buffer);
					strcat(newBuffer, allUsers[i].user_name);
					strcat(newBuffer, "\n");
					buffer = newBuffer;
				}
			}
			sendInt(newsize, 32, sock);
			write(sock, buffer, newsize);
			
			bzero((char *) client_input, sizeof(client_input));

			// Get target user
			read_size = receiveInt(16, sock);
			read(sock, client_input, read_size);
			char* target_user = (char *)malloc(strlen(client_input));
			strcpy(target_user, client_input);
			bzero((char *) client_input, sizeof(client_input));

			// Get message
			read_size = receiveInt(32, sock);
			read(sock, client_input, read_size);
			char* target_ptr = (char *)malloc(strlen(client_input));
			strcpy(target_ptr, client_input);
			bzero((char *) client_input, sizeof(client_input));

			bzero((char*)&response_message, sizeof(response_message));
			// Send if target user is online and exists
			if ((i = userOnline(target_user)) != -1){
				target_ptr = formatMessage(target_ptr, "C");
				sendInt(strlen(target_ptr), 32, allUsers[i].sd);
				write(allUsers[i].sd, target_ptr, strlen(target_ptr));

				response_message = "Message was sent!\n";
			} else {
				response_message = "Target user did not exist or was not online.\n";
			}
			
			// Send acknowledgment
			response_message = formatMessage(response_message, "C");
			sendInt(strlen(response_message), 32, sock);
			write(sock, response_message, strlen(response_message));
		
		} else {
			printf("That operation doesn't exist. Please try again.");
		}

		bzero((char *) client_message, sizeof(client_message));
	}
}

