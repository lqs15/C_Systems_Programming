/* this file expanded from given makeargv.h and given util.h from PA1 and PA2 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Stucture for candidates
typedef struct candidate {
	char* name;
	int count;
	struct candidate * next;
} cand_t;


// Structure for every node
typedef struct node{
	char* name;
	struct node** children;
	int num_children;
	struct node* parent;
	int result_file_created; // TRUE or FALSE
	pthread_mutex_t* node_lock;
	char* path_to_node;
	/* status
	 *   0 : Has not been picked up by a thread
	 *   1 : Has been picked up by a thread
	 *   2 : Done with this node (all votes counted and/or aggregated)
	 */
	int status;
	char* input_path;
	cand_t * candidates;
	/* number of times the result file for this node has been written to,
	 * this node can "move on" once it has been written to by all its
	 * children (i.e. num_times_written_to == num_children
	 */
	int num_times_written_to;
} node_t;


typedef struct leaf_queue {
	int num_leaves;
	node_t** leaf_nodes;
	pthread_mutex_t* q_lock;
	int* retrieved; // TRUE or FALSE
} leaf_q;


FILE* file_open(char* file_name);
char* read_line(char* buffer, FILE* fp);
int isspace(int c);

int makeargv(const char*s, const char *delimiters, char ***argvp){

	int error;
	int i;
	int numtokens;
	const char *snew;
	char *t;

	if ((s == NULL) || (delimiters == NULL) || (argvp == NULL)){

		errno = EINVAL;
		return -1;

	}

	*argvp = NULL; // already assigned as a new var, just blanking out

	snew = s + strspn(s, delimiters);

	if ((t = malloc(strlen(snew) + 1)) == NULL)
		return -1;

	strcpy(t, snew);

	numtokens = 0;

	if (strtok(t, delimiters) != NULL) // count number of tokens in s
		for (numtokens = 1; strtok(NULL, delimiters) != NULL; numtokens++);

	// create arg array for pointers to tokens
	if ((*argvp = malloc((numtokens + 1)*sizeof(char *))) == NULL){
		error = errno;
		free(t);
		errno = error;
		return -1;
	}

	// insert pointers to tokens into the arg array
	if (numtokens == 0)
		free(t);

	else{
		strcpy(t, snew);
		**argvp = strtok(t, delimiters);
		for(i = 1; i < numtokens; i++)
			*((*argvp) + i) = strtok(NULL, delimiters);
	}

	*((*argvp) + numtokens) = NULL; // put in final NULL pointer

	return numtokens;
}


// prints node graph depth first search
void printgraph(node_t* root) {
	if ( root != NULL ) {
		printf("Node %s\n", root->name);

		if ( root->parent == NULL ) {
			printf("- does not have a parent\n");
		} else {
			// TODO : this causes seg fault
			printf("- has parent, parent is: %s\n", (root->parent)->name);
		}

		printf("- has %d children\n", root->num_children);
		int i;
		for (i = 0; i < root->num_children; i++) {
			printgraph( (root->children)[i] );
		}
	}
}


/*
 * combines a filename to a pre-existing path
 * path = path/to/ OR path/to
 * file = myfile.txt
 * RETURN: path/to/myfile.txt
 */

char* makeFilePath(char* path, char* filename){
	char* nextPath = (char*) malloc(sizeof(char)*4096);  //4096 is linux's max path limit
	nextPath[0] = '\0';
	strcat(nextPath,path);
	if (nextPath[strlen(nextPath) - 1] != '/')
		strcat(nextPath,"/");
	strcat(nextPath,filename);
	return nextPath;
}

void writeLog(char* node, long tid, char* status,char* log_name, pthread_mutex_t* log_lock){
	char* logged = (char*) malloc(sizeof(char)*4096);  //4096 is linux's max path limit
	logged[0] = '\0';
	//change thread id from long to char*
	const int n = snprintf(NULL, 0, "%lu", tid);
	char buf[n+1];
	int tid_int = snprintf(buf, n+1, "%lu", tid);
	strcat(logged,node);
	strcat(logged,":");
	strcat(logged,buf);
	strcat(logged,":");
	strcat(logged,status);
	strcat(logged,"\n");
	pthread_mutex_lock(log_lock);
	FILE *f = fopen(log_name, "a");
  fputs(logged, f);
	fclose(f);
	pthread_mutex_unlock(log_lock);
	free(logged);
}

void freemakeargv(char **argv) {
   if (argv == NULL)
      return;
   if (*argv != NULL)
      free(*argv);
   free(argv);
}
