/* Authors : Mitchell Luhm, Huangying Kuang */
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
 * Opens file located at filePath,
 * Write a newline with the winner's name
 */
void fileAppend(char* filePath, char* winner) {
  FILE *f = fopen(filePath, "a");
  fprintf(f, "\nWinner:%s\n", winner);
  fclose(f);
}

/*
 * Opens the Root vote output file, determines the winner
 */
void findWinner(char* filePath) {
  int fileread = open(filePath,O_RDWR);
  if (fileread == -1) {
    perror("Error opening file in findWinner\n");
    exit(1);
  }
  char* buf = (char*) malloc(sizeof(char)*MAX_IO_BUFFER_SIZE);
  int nread = read(fileread,buf,MAX_IO_BUFFER_SIZE);
  if (nread == -1) {
    perror("Error reading file in findWinner\n");
    exit(1);
  }

  //divide file into candidates
  char** candidates;
  int length = makeargv(buf, ",", &candidates);
  // see if there are even candidates
  if (length < 1) {
    // THERE ARE NO CANDIDATES IN ROOT's OUTPUT COUNT
    perror("Root output file is empty. Are all votes.txt files blank?");
    exit(1);
  }

  // There are candidates, see who won!
  int max_votes = -1;
  char* max_candiate = "";

  int i;
  for (i = 0; i < length; i++) {
    // divide the candidate and vote total
    char** name_vote;
    int parts = makeargv(candidates[i], ":", &name_vote);
    int number = atoi(name_vote[1]);
    // if this candidate has more votes than current max,
    // set them as the new max_candidate
    if (number >= max_votes) {
      max_votes = number;
      max_candiate = name_vote[0];
    }
  }
  fileAppend(filePath, max_candiate);
  printf("%s\n", filePath); // REQUIRED STDOUT PRINT OF PATH
  freemakeargv(candidates);
  free(max_candiate);
}

int main(int argc, char **argv) {

  // we can have one or zero arguments
	if (argc > 2){
		printf("Expected zero or one program argument: a path to directory, ");
		printf("but the progam was given %d arguments.\n", argc-1);
		return -1;
	}
	//check if root_path specified
  char* root_path;
  if (argc == 2) {
    root_path = argv[1];
  } else {
    // get working directory
    char wd[1024];
    getcwd(wd, sizeof(wd));
    root_path = wd;
  }

  // CALL AGGREGATE VOTES ON ROOT
  pid_t child_pid = fork();

  if (child_pid > 0) {
    // in parent
    wait(NULL);
    // Construct the output file for reading,
    // writing, and then printing its path
    char* deep_dir = getDeepestDirectory(root_path);
    char* output_name = strcat(deep_dir, ".txt");
    root_path =
      makeFilePath(root_path, output_name);
    // append WINNER:CANDIDATE onto last line
    findWinner(root_path);
  } else if (child_pid == 0) {
    // in child
    // do work needed for parent process
    // supress printing in child process
		int fd = open("/dev/null", O_WRONLY);
		int redirect = dup2(fd, 1);    /* make stdout a copy of fd (> /dev/null) */
    if (redirect == -1) {
      perror("dup2 error\n");
      exit(1);
    }
		close(fd);      /* close fd */
    execl("./Aggregate_Votes", "Aggregate_Votes", root_path, (char*) NULL);
    perror("Execv failed\n");
    exit(1);
  } else {
    // fork failed
    perror("Fork failed\n");
    exit(1);
  }
  free(root_path);

  return 0;
}
