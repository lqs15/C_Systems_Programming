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


int incrementCand(node_t* candidates,char* name,int counter){
	//adding candidate for first time
	if (candidates->name == NULL){
		//candidates->name = name;
		candidates->name = (char*) malloc(sizeof(char)*1024);
		strcpy(candidates->name,name);
		candidates->count = counter;
		//printf("!! name: %s\n",candidates->name);
		//printf("!! count: %d\n",candidates->count);
	}
	else{
		//check if candiadte has already been added
		node_t* pointer = candidates;
		while(pointer != NULL){

			if (strcmp(pointer->name,name)==0){
				pointer->count += counter;
				//printf("!! name: %s\n",pointer->name);
				//printf("!! count: %d\n",pointer->count);
				return 1;
			}
			if (pointer->next == NULL) break;
			pointer = pointer->next;
		}
		node_t* newCandidate = (node_t*) malloc(sizeof(node_t*));
		newCandidate->name = (char*) malloc(sizeof(char)*1024);
		strcpy(newCandidate->name,name);
		newCandidate->count = counter;

		pointer->next = newCandidate;
		//printf("!! name: %s\n",pointer->next->name);
		//printf("!! count: %d\n",pointer->next->count);
	}
	return 1;
}

void aggregateSubNodes(char* path){
	//printf("-------- start aggregating ----------\n");
	//printf("------------ %s-----------------\n",path);
	DIR *dir = safeOpenDir(path);
	struct dirent *entry;
	node_t* candidates = (node_t*) malloc(sizeof(node_t*));

	while((entry = readdir(dir))!= NULL){
		if (validFolder(entry)== 1){
			// produce new filename
			char* votesPath = makeFilePath(path,entry->d_name);
			votesPath = makeFilePath(votesPath,entry->d_name);
			strcat(votesPath,".txt");

			//read file
			int fileread = open(votesPath,O_RDWR);
      if (fileread == -1) {
        perror("Error opening file in Aggregate_Votes\n");
        exit(1);
      }
			char* buf = (char*) malloc(sizeof(char)*MAX_IO_BUFFER_SIZE);
			int nread = read(fileread,buf,MAX_IO_BUFFER_SIZE);
      if (nread == -1) {
        perror("Error happening reading a file in Aggregate_Votes\n");
        exit(1);
      }

			//parse file
			char** records;
			int num_can = makeargv(buf, ",", &records);
			int i;
			for (i = 0; i < num_can; i++){
				char* temp = malloc(sizeof(char*));
				strcpy(temp,records[i]);
				char** candidate;

				int length = makeargv(temp, ":", &candidate);

				int success = incrementCand(candidates,candidate[0],atoi(candidate[1]));
				//freemakeargv(candidate); THIS FREE IS VERY PROBLEMATIC! VERY!
			}
			//freemakeargv(records);
			//free(buf);
		}
	}

	char* output_file_name = makeOutputFile(candidates, path);
  // CHILD SHOULD NOT PRINT (THIS is printing when VoteCounter run)
  printf("%s\n", output_file_name);

	//TODO free node list
}



int main(int argc, char **argv){

	if (argc != 2){
		printf("Expected one program argument: a path to directory, ");
		printf("but the progam was given %d arguments.\n", argc-1);
		return -1;
	}
	//check if current path is leaf
	char* path = argv[1];
	int leaf = isLeafDirectory(path);

	if (!leaf) {
		DIR *dir = safeOpenDir(path);
		struct dirent *entry;
		pid_t pid;
		while((entry = readdir(dir))!= NULL){
			//as directory
			if (validFolder(entry)== 1){

				char* nextPath = makeFilePath(path,entry->d_name);

				pid = fork();
				if (pid == 0) {
					// redirect stdout to nothing
					int fd = open("/dev/null", O_WRONLY);
          if (fd == -1) {
            perror("Error opening file in Aggregate_Votes\n");
            exit(1);
          }
					int redirect = dup2(fd, 1);    /* make stdout a copy of fd (> /dev/null) */
          if (redirect == -1) {
            perror("dup2 error\n");
            exit(1);
          }
					close(fd);      /* close fd */
					execl("./Aggregate_Votes","Aggregate_Votes",nextPath,(char*) NULL);
					perror("Execv failed\n");
					exit(1);
				}
				else if (pid > 0) {
					wait(0);
					int status = 0;
					pid_t wpid;
					while ((wpid = wait(&status)) > 0);
				} else if (pid == -1) {
					perror("Fork Failed\n");
					exit(1);
				}
			}
		}
		aggregateSubNodes(path);
	}
	else {
    // redirect stdout to nothing
		int fd = open("/dev/null", O_WRONLY);
    if (fd == -1) {
      perror("Error opening file in Aggregate_Votes\n");
      exit(1);
    }
		int redirect = dup2(fd, 1);    /* make stdout a copy of fd (> /dev/null) */
    if (redirect == -1) {
      perror("dup2 error\n");
      exit(1);
    }
		close(fd);      /* close fd */
		execl("./Leaf_Counter","Leaf_Counter",path,(char*) NULL);
    perror("Exec error in Aggreage_Votes\n");
    exit(1);
	}
  return 0;
}
