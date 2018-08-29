#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>


/* The maximum amount of bytes for a file name */
#define MAX_FILE_NAME_SIZE 255

/* The maximum amount of bytes for each I/O operation */
#define MAX_IO_BUFFER_SIZE 1024

// node strucutre used to help associate votes with candidates
typedef struct node {
  char* name;
  int count;
  struct node * next;
} node_t;


/**********************************
*
* Taken from Unix Systems Programming, Robbins & Robbins, p37
*
*********************************/
int makeargv(const char *s, const char *delimiters, char ***argvp) {
   int error;
   int i;
   int numtokens;
   const char *snew;
   char *t;

   if ((s == NULL) || (delimiters == NULL) || (argvp == NULL)) {
      errno = EINVAL;
      return -1;
   }
   *argvp = NULL;
   snew = s + strspn(s, delimiters);
   if ((t = malloc(strlen(snew) + 1)) == NULL)
      return -1;
   strcpy(t,snew);
   numtokens = 0;
   if (strtok(t, delimiters) != NULL)
      for (numtokens = 1; strtok(NULL, delimiters) != NULL; numtokens++) ;

   if ((*argvp = malloc((numtokens + 1)*sizeof(char *))) == NULL) {
      error = errno;
      free(t);
      errno = error;
      return -1;
   }

   if (numtokens == 0)
      free(t);
   else {
      strcpy(t,snew);
      **argvp = strtok(t,delimiters);
      for (i=1; i<numtokens; i++)
         *((*argvp) +i) = strtok(NULL,delimiters);
   }
   *((*argvp) + numtokens) = NULL;
   return numtokens;
}

/**********************************
*
* Taken from Unix Systems Programming, Robbins & Robbins, p38
*
*********************************/
void freemakeargv(char **argv) {
   if (argv == NULL)
      return;
   if (*argv != NULL)
      free(*argv);
   free(argv);
}

char *trimwhitespace(char *str) {
  char *end;
  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;

  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

/*
 * combines a filename to a pre-existing path
 * path = path/to/ OR path/to
 * file = myfile.txt
 * RETURN: path/to/myfile.txt
 */
char* makeFilePath(char* path, char* filename){
	char* nextPath = (char*) malloc(sizeof(char)*MAX_FILE_NAME_SIZE);
	nextPath[0] = '\0';
	strcat(nextPath,path);
  int length = strlen(nextPath);
  if (nextPath[length-1] != '/') {
    strcat(nextPath,"/");
  }
	strcat(nextPath,filename);
	return nextPath;
}

/*
 * Returns a string of just the deepest directory of a path
 * path = path/to/dir/deepdir
 * RETURN: deepdir
 */
char* getDeepestDirectory(char* path) {
  int path_length = strlen(path);
  int i, ch;
  // we don't want to return a / at the end of the dir
  // so we can easily add an extension like .txt
  if (path_length > 0 && path[path_length-1] == '/') {
    path[path_length-1] = '\0';
    path_length--;
  }
  for (i = path_length-1; i >= 0; i--) {
    if (path[i] == '/') {
      // got to start of deepest directory name
      char* directory = (char*) malloc(sizeof(char)*256);
      int index = 0;
      // copy subsection of path to directory
      for (ch = i+1; ch < path_length; ch++) {
        directory[index] = path[ch];
        index++;
      }
      // terminate the directory string
      directory[index] = '\0';
      return directory;
    }
    if (i == 0) {
      // we got here if there are no /'s in the path
      // so we just return what the path is coped into directory
      char* directory = (char*) malloc(sizeof(char)*path_length);
      int j;
      for (j = 0; j < path_length; j++) {
        directory[j] = path[j];
      }
      return directory;
    }
  }
  return path; // ERROR
}
/*
 Take: a path to the directory for check
 Return: 0 if not leaf and 1 if leaf
 Criteria of leaf: if the directory doesn't have any sub-directories
*/
int isLeafDirectory(char* path){
	int found = 0;
	int leaf = 1;
	DIR *dir;
	struct dirent *entry;
	if ((dir = opendir(path)) == NULL)
		perror("opendir() error");
	else {
		while ((entry = readdir (dir))!=NULL){
			//contain votes.txt
			if (strcmp(entry->d_name, "votes.txt")==0){
				int fileread = open(entry->d_name,O_RDWR);
				found = 1;
			}
			//have subdirectories
			if (entry->d_type == DT_DIR
			   && strcmp(entry->d_name,".") != 0
			   && strcmp(entry->d_name,"..") != 0) {
				leaf = 0;
			}
		}
	}
	if (!found) {
		return 0;
	}
	if (!leaf){
		return 0;
	}
	return 1;
}


DIR* safeOpenDir(char* path){

	DIR *dir;
	if ((dir = opendir(path)) == NULL){
		perror("opendir() error");
    // can we exit here?
	}
  return dir;
}


int validFolder (struct dirent* entry){
	if (entry->d_type == DT_DIR
		 && strcmp(entry->d_name,".") != 0
		 && strcmp(entry->d_name,"..") != 0){
			return 1;
	}
	else {
		return 0;
	}

}


/*
 * Receives node list of candidates with their
 *   NAME and COUNT
 * Make a .txt file
 *   NAME1:COUNT1,NAME2:COUNT2, ...
 */
char* makeOutputFile(node_t* candidates, char* path) {
  //printf("path is: %s\n", path);
  char* output_base = getDeepestDirectory(path);
  strcat(output_base, ".txt");
  char* fname = makeFilePath(path, output_base);
  //printf("util.h makeOutputFile fname is : %s\n", fname);

	FILE *f = fopen(fname, "w");
	if (f == NULL)
	{
		perror("makeOutputFile(): error opening file\n");
		exit(1);
	}

  node_t* current_candidate = candidates;
  // if node list empty, then don't write null counts to file
  if (current_candidate->name != NULL) {
    // write to file getting candidate information
    while (current_candidate != NULL) {
      fprintf(f, "%s:", current_candidate->name);
      // if we are at last candidate, don't write a comma
      if (current_candidate->next == NULL) {
        fprintf(f, "%d", current_candidate->count);
      } else {
        fprintf(f, "%d,", current_candidate->count);
      }
      current_candidate = current_candidate->next;
    }
  }

	fclose(f);
  return fname;
}


