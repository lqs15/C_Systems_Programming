#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include "makeargv.h"

/* The maximum amount of bytes for a file name */
#define MAX_FILE_NAME_SIZE 255

/* The maximum amount of bytes for each I/O operation */
#define MAX_IO_BUFFER_SIZE 1024

// node strucutre used to help associate votes with candidates
typedef struct candidate_node {
  char* name;
  int count;
  struct candidate_node * next;
} cand_node;

/*
 * find node in list if exists
 * - if it doesn't exist, add it
 * increment its count attribute
 */
void incrementCandidate(cand_node* candidates, char* name){
	if (strcmp(name,"\n")==0) return;
	//adding candidate for first time
	if (candidates->name == NULL){
		candidates->name = (char*) malloc(sizeof(char)*1024);
		strcpy(candidates->name, name);
		candidates->count ++;
		return;
	}
	//check if candiadte has already been added
	cand_node* pointer = candidates;
	cand_node* lastP;
	while(pointer != NULL){
		if (strcmp(pointer->name, name)==0){
			pointer->count ++;
			return;
		}
		lastP = pointer;
		pointer = pointer->next;
	}
	//if not added, create new node
	cand_node* newCandidate = (cand_node*) malloc(sizeof(cand_node));
	newCandidate->name = (char*) malloc(sizeof(char)*1024);
	strcpy(newCandidate->name, name);
	newCandidate->count ++;
	lastP->next = newCandidate;
	return;
}

/*
 * takes the filePath to read a votes.txt
 * creates the node structure of candidate names and their
 * vote counts. This node structure is passed to
 * makeOutputFile(), to produce the output text files for
 * a given leaf.
 */
char* readVotes(char* filePath) {
	int fileread = open(filePath,O_RDWR);
  if (fileread == -1) {
    perror("");
    exit(1);
  }

	char* buf = (char*) malloc(sizeof(char)*MAX_IO_BUFFER_SIZE);
	int nread = read(fileread,buf,MAX_IO_BUFFER_SIZE);
  if (nread == -1) {
    perror("");
    exit(1);
  }

	//divide file into lines
	char** line;
	int length = makeargv(buf, "\n", &line);
	int i;
	cand_node* candidates = (cand_node*) malloc(sizeof(cand_node));
	for (i = 0; i < length; i++){
		incrementCandidate(candidates, line[i]);
	}

  if (i == 0) {
    // empty votes.txt
    printf("There was an empty votes.txt file\n");
  }
  char* read_data = (char*) malloc(sizeof(char)*235);
  read_data[0] = '\0';
  cand_node * current_cand = candidates;
  while (current_cand != NULL) {
	// make string data
	// "A:1,B:2,....
	// buffer for int to string conversion
	char int_to_string[16];
	sprintf(int_to_string, "%d", current_cand->count);

	// concat to the read_data string
	strcat(read_data, current_cand->name);
	strcat(read_data, ":");
	strcat(read_data, int_to_string);

	// add , only if there is another candidate
	if (current_cand->next != NULL) {
		strcat(read_data, ",");
	} else {
		break;
	}
	current_cand = current_cand->next;
  }
  return read_data;
}

char* getCommandsFile(char* path) {
	int path_length = strlen(path);
	int slash_i = 0;
	int i;
	for (i = 0; i < path_length; i++) {
		if ( path[i] == '/' ) slash_i = i;
	}

	if ( slash_i ) {
		// there exists a slash
		int file_length = path_length - (slash_i + 1);
		char* file_name = (char*) malloc(sizeof(char)*file_length);
		for (i = 0; i < file_length; i++) {
			file_name[i] = path[slash_i + i + 1];
		}
		return file_name;
	} else {
		return path;
	}

}

char* getPathToFile(char* path) {
	int path_length = strlen(path);
	int slash_i = 0;
	int i;
	for (i = 0; i < path_length; i++) {
		if ( path[i] == '/' ) slash_i = i;
	}

	if ( slash_i ) {
		// there exists a slash
		int subpath_length = slash_i + 1;
		char* path_name = (char*) malloc(sizeof(char)*subpath_length);
		for (i = 0; i < subpath_length; i++) {
			path_name[i] = path[i];
		}
		return path_name;
	} else {
		return "";
	}

}
