
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"

/*
 * find node in list if exists
 * - if it doesn't exist, add it
 * increment its count attribute
 */
void incrementCandidate(node_t* candidates, char* name){
	if (strcmp(name,"\n")==0) return;
	//adding candidate for first time
	if (candidates->name == NULL){
		candidates->name = (char*) malloc(sizeof(char)*1024);
		strcpy(candidates->name, name);
		candidates->count ++;
		return;
	}
	//check if candiadte has already been added
	node_t* pointer = candidates;
	node_t* lastP;
	while(pointer != NULL){
		if (strcmp(pointer->name, name)==0){
			pointer->count ++;
			return;
		}
		lastP = pointer;
		pointer = pointer->next;
	}
	//if not added, create new node
	node_t* newCandidate = (node_t*) malloc(sizeof(node_t*));
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
void readVotes(char* filePath, char* path){
	int fileread = open(filePath,O_RDWR);
  if (fileread == -1) {
    perror("Leaf_Counter:readVotes, error opening file\n");
    exit(1);
  }

	char* buf = (char*) malloc(sizeof(char)*MAX_IO_BUFFER_SIZE);
	int nread = read(fileread,buf,MAX_IO_BUFFER_SIZE);
  if (nread == -1) {
    perror("Leaf_Counter:readVotes, error reading file\n");
    exit(1);
  }

	//divide file into lines
	char** line;
	int length = makeargv(buf, "\n", &line);
	int i;
	node_t* candidates = (node_t*) malloc(sizeof(node_t*));
	for (i = 0; i < length; i++){
		incrementCandidate(candidates, line[i]);
	}

  if (i == 0) {
    // empty votes.txt
    printf("There was an empty votes.txt file\n");
  }

	char* output_file_name = makeOutputFile(candidates, path);
  // not to be printed if this is a child process
  printf("%s\n", output_file_name);

	freemakeargv(line); //not sure about this function from util.h
	//TODO free node list
}

int main(int argc, char **argv){
	if (argc != 2){
		printf("Expected one program argument: a path to directory, ");
		printf("but the progam was given %d arguments.\n", argc-1);
		return -1;
	}
	//check if votes.txt exists
	char* path = argv[1];
	int found = isLeafDirectory(path);
	if (!found) {
		printf("Not a leaf node.\n");
		return -1;
	}

	//read content of votes.txt
	char* filePath = makeFilePath(path,"votes.txt");

  readVotes(filePath, path);

  return 0;
}
