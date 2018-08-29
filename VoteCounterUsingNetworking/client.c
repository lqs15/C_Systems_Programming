/* file : client.c */

/* Authors : Huangying Kuang, Mitchell Luhm */

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
//#include "makeargv.h"
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "client_util.h"

/* The maximum amount of bytes for a file name */
#define MAX_FILE_NAME_SIZE 255

/* The maximum amount of bytes for each I/O operation */
#define MAX_IO_BUFFER_SIZE 1024

#define NUM_ARGS 3

char** cmds;

void putWhitespaceFromTo(int start, int end, char* msg) {
	int i;
	for (i = start; i < end; i++) msg[i] = ' ';
}

void fillMessageSkeleton(char* msg) {
	putWhitespaceFromTo(0, 2, msg);
	msg[2] = ';';
	putWhitespaceFromTo(3, 18, msg);
	msg[18] = ';';
	putWhitespaceFromTo(19, 255, msg);
	msg[255] = '\0';
}

int makeData(char* data, char* msg, char* full_path) {
	// open data files
	char* subpath = getPathToFile(full_path);
	int subpath_len = strlen(subpath);
	int data_len = strlen(data);
	int total_len = subpath_len + data_len;
	char* path_to_votes = (char*) malloc(sizeof(char)*(total_len));
	path_to_votes[0] = '\0';
	strcat(path_to_votes, subpath);
	strcat(path_to_votes, data);
	char* read_data = readVotes(path_to_votes);

	int data_length = strlen(read_data);
	int i;
	for (i = 0; i < data_length; i++) {
		msg[19 + i] = read_data[i];
	}
	// end entire message here
	msg[19 + i] = '\0';

	return 0;
}

int makeRegionName(char* name, char* msg) {
	int name_length = strlen(name);
	int i;
	for (i = 0; i < name_length; i++) {
		msg[3 + i] = name[i];
	}

	return 0;
}

/* return 1 when done */
int makeResponseCode(char* response, char* msg) {
	if ( strcmp(response, "Return_Winner") == 0 ) {
		msg[0] = 'R';
		msg[1] = 'W';
		msg[19] = '\0';
		return 1;
	} else if ( strcmp(response, "Count_Votes") == 0 ) {
		msg[0] = 'C';
		msg[1] = 'V';
	} else if ( strcmp(response, "Open_Polls") == 0 ) {
		msg[0] = 'O';
		msg[1] = 'P';
	} else if ( strcmp(response, "Add_Votes") == 0 ) {
		msg[0] = 'A';
		msg[1] = 'V';
	} else if ( strcmp(response, "Remove_Votes") == 0 ) {
		msg[0] = 'R';
		msg[1] = 'V';
	} else if ( strcmp(response, "Close_Polls") == 0 ) {
		msg[0] = 'C';
		msg[1] = 'P';
	} else {
		// this will result in an unhandled command, this completely destroys
		// the format of the client's message to server provided the command
		// is greater than two characters.
		int r_len = strlen(response);
		int i;
		for (i = 0; i < r_len; i++) {
			msg[i] = response[i];
		}
		msg[i] = ';';
		msg[i+1] = '\0';
		return 1;
	}

	return 0;
}

char* commandToMessage(char* command, char* full_path) {
	// malloc max space for a single message
	char* msg = (char*) malloc(sizeof(char)*256);
	fillMessageSkeleton(msg);
	char** line;
	int length = makeargv(command, " ", &line);
	int i;
	for (i = 0; i < length; i++){
		if ( i ==  0 ) {
			// if Return_Winner we are done
			if ( makeResponseCode(line[i], msg) ) break;
		} else if ( i == 1 ) {
			makeRegionName(line[i], msg);
			// if Add_Votes, Remove_Votes, Open_Polls, or Close_Polls we are done
			if ( (!(msg[0] != 'C' && msg[1] == 'V')) || msg[1] == 'P' ) {
				msg[19] = '\0';
				break;
			}
		} else if ( i == 2 ) {
			makeData(line[i], msg, full_path);
			break;
		}
	}
	return msg;
}

int readCommandFile(char* filePath) {
	int fileread = open(filePath,O_RDWR);
	if (fileread == -1) {
		perror("readCommandFile() in client.c failed to open commands");
		exit(1);
	}

	char* buf = (char*) malloc(sizeof(char)*MAX_IO_BUFFER_SIZE);
	int nread = read(fileread,buf,MAX_IO_BUFFER_SIZE);
	if (nread == -1) {
		perror("readCommandFile() in client.c failed to read commands");
		exit(1);
	}

	//divide file into lines
	char** line;
	int length = makeargv(buf, "\n", &line);
	// malloc a char* spot for every command
	cmds = (char**) malloc(sizeof(char*)*length);
	int i;
	for (i = 0; i < length; i++){
		// malloc space for a single message
		cmds[i] = (char*) malloc(sizeof(char)*256);
		char* msg = commandToMessage(line[i], filePath);
		strcpy(cmds[i], msg);
		//free(msg);
		//cmds[i] = "";
		//strcat(cmds[i], msg);
	}

	// return number of commands
	return length;
}

int main(int argc, char **argv) {

	// number of commands
	int n_commands = readCommandFile(argv[1]); // commands);

  if (argc < NUM_ARGS + 1) {

		printf("Wrong number of args, expected %d, given %d\n", NUM_ARGS, argc - 1);
		exit(1);
	}
  char* req_file_name = argv[1];
  int server_ip = atoi(argv[2]);
  char* server_ip_s = argv[2]; // used for printing
  int server_port = atoi(argv[3]);


	// Create a TCP socket.
	int sock = socket(AF_INET , SOCK_STREAM , 0);

	// Specify an address to connect to (we use the local host or 'loop-back' address).
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(server_port);
	address.sin_addr.s_addr = inet_addr("127.0.0.1");

	// Connect it.
	if (connect(sock, (struct sockaddr *) &address, sizeof(address)) == 0) {

		/*
		// Buffer for data.
		char buffer[256];
		printf("Initiated connection with server at %d:%d\n",server_ip,server_port);
    	write(sock,"buffer",256);
		close(sock);
		*/
		printf("Initiated connection with server at %s:%d\n",server_ip_s,server_port);
			char message[256];
			char message2[256];
		int cmd_i;
		for (cmd_i = 0; cmd_i < n_commands; cmd_i++) {
			//printf("about to send %s to server\n", cmds[cmd_i]);
			// Buffer for data.
			//char buffer[256];
			printf("Sending request to server: %s\n", cmds[cmd_i]);
			write(sock, cmds[cmd_i], 256);

			// wait for server to respond now
			// while n read == 0
			// Read from the socket and print the contents.
			/*int nread = read(sock, message, 256);
			while ( nread == 0 ){
				//printf("%s\n", message);
				printf("waiting in while\n");
				//nread = read(sock, message, 256);
				nread = read(sock, message, 256);
			}
			*/
			int nread = recv(sock, message, 256, 0);
			printf("Received response from server: %s\n", message);
		}
		close(sock);
		printf("Closed connection with server at %s:%d\n",server_ip_s,server_port);

	} else {

		perror("Connection failed!");
	}

	return 0;
}
