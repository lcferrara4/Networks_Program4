/* Networks Program 4
 * Lauren Ferrara - lferrara
 * Charlie Newell - cnewell1
 *
 * Client side of Chat Service
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

// A structure to hold messages that need processing
// Basic idea of queue implementation taken from geeksforgeeks example
struct MessageQueue {
	int front, rear, size;
	unsigned capacity;
	char** array;
};
 
// Create a queue of given capacity. 
struct MessageQueue* createQueue(unsigned capacity) {
	struct MessageQueue* queue = (struct MessageQueue*) malloc(sizeof(struct MessageQueue));
	queue->capacity = capacity;
	queue->front = queue->size = 0; 
	queue->rear = capacity - 1;
	queue->array = (char**) malloc(queue->capacity * sizeof(char*));
	return queue;
}
 
// Queue is full when size becomes equal to the capacity 
int isFull(struct MessageQueue* queue)
{	return (queue->size == queue->capacity);  }
 
// Queue is empty when size is 0
int isEmpty(struct MessageQueue* queue)
{	return (queue->size == 0); }
 
// Function to add an item to the queue.  
// It changes rear and size
void push_back(struct MessageQueue* queue, char* message) {
	if (isFull(queue))
		return;
	queue->rear = (queue->rear + 1)%queue->capacity;
	queue->array[queue->rear] = message;
	queue->size = queue->size + 1;
}
 
// Function to remove an item from queue. 
// It changes front and size
char* pop_front(struct MessageQueue* queue) {
	if (isEmpty(queue))
		return "";
	char* message = queue->array[queue->front];
	queue->front = (queue->front + 1)%queue->capacity;
	queue->size = queue->size - 1;
	return message;
}
 
// Function to get front of queue
char* front(struct MessageQueue* queue) {
	if (isEmpty(queue))
		return "";
	return queue->array[queue->front];
}
 
// Function to get rear of queue
char* rear(struct MessageQueue* queue) {
	if (isEmpty(queue))
		return "";
	return queue->array[queue->rear];
}

// threading function
void *connection_handler(void *);

#define BUFSIZE 4096

// Global variables
int EXIT = 0;
struct MessageQueue* QUEUE;
int CONFIRMED = 0; // once user is confirmed

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
	//printf("%i\n", size);
        return size;
}

void sendInt(int x, int bits, int socket) {
        if (bits == 32) {
                int32_t conv = htonl(x);
                char *data = (char*)&conv;
                int size_int = sizeof(conv);
                if (write(socket, data, size_int) < 0) {
                        perror("FTP Server: Error sending size of directory listing\n");
                        exit(1);
                }

        } else if (bits == 16) {
                int16_t conv = htons(x);
                char *data = (char*)&conv;
                int size_int = sizeof(conv);
                if (write(socket, data, size_int) < 0) {
                        perror("FTP Server: Error sending size of directory listing\n");
                        exit(1);
                }

        }
}

void sendFileDir(int socket, char *filename) {
	// Get file or directory name for operation
	printf("Enter file or directory name: ");
	scanf("%s", filename);		
	int size = strlen(filename);	

	sendInt(size, 16, socket);
	// Send filename
	if (write(socket, filename, size) < 0) {
		perror("FTP Client: Error writing to socket\n");
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

int main (int argc, char *argv[]) {

	// Variable declaration
	int port, s, size;
	char *server_name;
	char *user_name;
	struct hostent *server;
	struct sockaddr_in server_addr;
	char buffer[BUFSIZE];
	int opt = 1; /* 0 to disable options */
	char filename[BUFSIZE];

	// Check command line arguments
	if (argc != 4) {
		fprintf(stderr, "Usage: %s Server_Name [Port] [User_Name]\n", argv[0]);
		exit(1);
	}
	server_name = argv[1];
	port = atoi(argv[2]);
	user_name = argv[3];

	// Initialize global queue to hold messages to process
	QUEUE = createQueue(1000);

	// Translate server name to IP address
	server = gethostbyname(server_name);
	if (!server) {
		fprintf(stderr, "Chatclient: Unknown server %s\n", server_name);
		exit(1);
	}

	// Open socket
	if ((s = socket (PF_INET, SOCK_STREAM, 0)) < 0 ) {
		perror("Chatclient: Unable to open socket\n");
		exit(1);
	}

	// Load buffer
	bzero(buffer, BUFSIZE);

	// Build server address data structure
	bzero((char*) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	bcopy((char*) server->h_addr, (char *) &server_addr.sin_addr.s_addr, server->h_length);
	server_addr.sin_port = htons(port);

	setsockopt(s, SOL_SOCKET, SO_REUSEADDR,(char *)&opt, sizeof(int));

	// Connect to socket
	if (connect(s, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0 ) {
		perror("Chatclient: Unable to connect to socket\n");
		exit(1);
	}
	
	// Initialize message_handler thread
	pthread_t thread2;
	if (pthread_create( &thread2, NULL, connection_handler, (void*) &s) < 0) {
		perror("Chatclient: Could not create thread\n");
		exit(1);
	}

	// Send username to server
	sendInt(strlen(user_name), 16, s);
	write(s, user_name, strlen(user_name));
		
	// receive server message and print to user
	int server_message_size = receiveInt(32, s);
	char server_message[server_message_size];
	server_message[server_message_size] = '\0';
	read(s, server_message, server_message_size);
	push_back(QUEUE, server_message);

	/*
	printf("%s", server_message);
	
	// Scan the password and send to user
	scanf("%s", pass);
	int passSize = strlen(pass);
	sendInt(passSize, 16, s);	

	//write password to server
	if (write(s, pass, passSize) < 0) {
		perror("Error writing to socket\n");
		exit(1);
	}
	*/

	// Get and print server acknowledgement
	int server_response_size = receiveInt(32, s);
	char server_response[server_response_size];
	server_response[server_response_size] = '\0';
	read(s, server_response, server_response_size);
	push_back(QUEUE, server_response);

	while (!CONFIRMED) {}
	
	// Prompt user for operation state?
	// Collects all messages from socket
	while (!EXIT) {
		/*
		int message_size = receiveInt(32, s);
		char message[message_size];
		if (read(s, message, message_size) < 0) {
			perror("Chatclient: Error reading from socket\n");
			exit(1);
		}
		*/
	}

	pthread_join( thread2 , NULL);
}

// Parses and reacts to messages from socket
void *connection_handler(void *sock) {
	int s = *(int*)sock;
	char* message;
	char pass[100];

	// Wait for message
	while(isEmpty(QUEUE)) {}

	// Get password
	message = pop_front(QUEUE);
	printf("%s", message);
	
	// Scan the password and send to user
	scanf("%s", pass);
	int passSize = strlen(pass);
	sendInt(passSize, 16, s);	

	//write password to server
	if (write(s, pass, passSize) < 0) {
		perror("Error writing to socket\n");
		exit(1);
	}

	// Wait for message
	while(isEmpty(QUEUE)) {}

	// Get confirmation
	message = pop_front(QUEUE);
	printf("%s\n", message);

	if ( strncmp("Welcome", message, 7) != 0 ) {
		EXIT = 1;
	}

	CONFIRMED = 1;
	//printf("%s", message);
	//printf("Enter FTP operation: ");
	//bzero((char*)&operation, sizeof(operation));
	/*scanf("%s", pass);
	int passSize = strlen(pass);
	
	sendInt(passSize, 16, s);	

	if (write(s, pass, passSize) < 0) {
		perror("Error writing to socket\n");
		exit(1);
	}*/
	
	/*if (write(s, operation, size) < 0) {
		perror("FTP Client: Error writing to socket\n");
		exit(1);	
	}

		// Send message to server
		if (!strcmp(operation, "QUIT")) {
			// Exit client connection
			printf("FTP Client: Connection has been closed.\n");
			exit(0);
		} else if (!strcmp(operation, "LIST")) {
			// List directory at the server
			// Get directory listing size
			int listing_size = receiveInt(32, s);
			char listing[listing_size];
			if (read(s, listing, listing_size) < 0) {
				perror("FTP Client: Error reading from socket\n");
				exit(1);
			}
			printf("%s", listing);
		} else if (!strcmp(operation, "DWLD")) {

			// Download file
			struct timeval start_t, end_t;               
			
			// Get start time
			if(gettimeofday(&start_t, NULL)==-1){
				perror("FTP Client: Start Time Error!\n"); 
				exit(1); 
			}
 
			bzero((char*)&filename, sizeof(filename));
			sendFileDir(s, filename);
			
			int file_size = receiveInt(32, s);
			int total_recv = 0;
			int i = 0;
			int x;
			char inner_buffer[BUFSIZE];

			if(file_size == -1) {
				printf("The desired file does not exist\n");
				continue;
			
			} else {
				FILE *fp = fopen(filename, "a");
				int recv_len = 0;
				int total = 0;
				
				while((recv_len = recv(s, inner_buffer, BUFSIZE, 0)) > 0) {
					total += recv_len;
					if(total > file_size) {
						inner_buffer[file_size - (total - recv_len)] = '\0';
						x = fwrite(inner_buffer, sizeof(char), file_size - (total - recv_len), fp);
						break;
					}
					x = fwrite(inner_buffer, sizeof(char), recv_len, fp);
					bzero(inner_buffer, BUFSIZE);
					if (total >= file_size) break;
				}
				fclose(fp);
				
				
			}
			// Get end time
			if(gettimeofday(&end_t, NULL)==-1){
                    		perror("FTP Client: End Time Error!\n"); 
                    		exit(1);
				
                	}
                	double throughput = (end_t.tv_usec - start_t.tv_usec); 
                	printf("Download was successful. Throughput: %f microseconds\n", throughput);

		} else if (!strcmp(operation, "UPLD")) {
			// Upload file
			bzero((char*)&filename, sizeof(filename));
			sendFileDir(s, filename);
			// Upload file
			char inner_buffer[BUFSIZE];
			int x = getFileSize(filename);
			sendInt(x, 32, s);
			FILE *fp = fopen(filename, "r");		
			printf("before while\n");	

			int sent = 0;
			int total = 0;
			while(sent = fread(inner_buffer, sizeof(char), BUFSIZE, fp)) {
				total += sent;
				if(write(s, inner_buffer, sent) < 0) {
					perror("FTP Server: Error sending file\n");
					exit(1);
				}
				bzero(inner_buffer, BUFSIZE);
				if(total >= x) break;
			}
			fclose(fp);
		} else if (!strcmp(operation, "DELF")) {
			// Delete file
			bzero((char*)&filename, sizeof(filename));
			sendFileDir(s, filename);

			int confirm = receiveInt(32, s);
			if (confirm < 0) {
				printf("The file does not exist on server\n");
			} else {
				char inner_buffer[BUFSIZE];
				printf("Are you sure you want to delete? Yes or No: ");
				scanf("%s", inner_buffer);
				size = strlen(inner_buffer);
				if (write(s, inner_buffer, size) < 0) {
					perror("FTP Client: Error writing to socket\n");
					exit(1);
				}
				if (!strcmp(inner_buffer, "Yes")){
					confirm = receiveInt(32, s);
					if (confirm < 0) {
						perror("FTP Client: Error deleting the file\n");
						exit(1);
					} else {
						printf("File deleted successfully\n");
					}	
				} else {
					printf("Delete abandoned by the user!\n");
				}
			}
		} else if (!strcmp(operation, "MDIR")) {
			// Make directory
			bzero((char*)&filename, sizeof(filename));
			sendFileDir(s, filename);

			int confirm = receiveInt(32, s);
			if (confirm == -2) {
				printf("The directory already exists on server\n");
			} else if (confirm == -1) {
				printf("Error in making directory\n");
			} else {
				printf("The directory was successfully made\n");
			}
		} else if (!strcmp(operation, "RDIR")) {
			// Remove directory
			bzero((char*)&filename, sizeof(filename));
			sendFileDir(s, filename);

			int confirm = receiveInt(32, s);
			if (confirm == -1) {
				printf("The directory doesn't exist on server\n");
			} else {
				char inner_buffer[BUFSIZE];
				printf("Are you sure you want to delete? Yes or No: ");
				scanf("%s", inner_buffer);
				size = strlen(inner_buffer);
				if (write(s, inner_buffer, size) < 0) {
					perror("FTP Client: Error writing to socket\n");
					exit(1);
				}
				if (!strcmp(inner_buffer, "Yes")){
					confirm = receiveInt(32, s);
					if (confirm < 0) {
						perror("FTP Client: Error deleting the directory\n");
						exit(1);
					} else {
						printf("Directory deleted successfully\n");
					}
				} else {
					printf("Delete abandoned by the user!\n");
				}
			}
		} else if (!strcmp(operation, "CDIR")) {
			// Change to a different directory
			bzero((char*)&filename, sizeof(filename));
			sendFileDir(s, filename);

			int confirm = receiveInt(32, s);
			if (confirm == -2) {
				printf("The directory doesn't exist on server\n");
			} else if (confirm == -1) {
				printf("Error in changing directory\n");
			} else {
				printf("Changed current directory\n");

			}
		}
	}
	exit(0);*/
}

