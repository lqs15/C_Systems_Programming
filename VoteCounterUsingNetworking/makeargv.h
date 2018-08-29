/*  this file expanded from given makeargv.h and given util.h from PA1 and PA2 */

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
	/*  0 : polls are closed (have not been opened and closed)
	 *  1 : polls are open
	 * -1 : polls were open, but now closed.
	 */
	int is_polling_open;
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


/** Find a node by its name
 * INPUT : Sting of a node name,
 *         Root node
 * OUTPUT : the node_t* of node with name specified by input
 */
node_t* getNodeByName(char* name, node_t* n) {
	if ( strcmp( n->name, name ) == 0 ) {
		// node found
		return n;
	}

	node_t * current_node = n;

	int n_children = current_node->num_children;
	int i;
	for (i = 0 ; i < n_children; i++) {
 		// recursively explorer that node's children
		node_t * r_node = getNodeByName(name, (current_node->children)[i]);
		if (r_node == NULL) {
		}
		else if ( strcmp( r_node->name, name ) == 0 ) {
			// node found
			return r_node;
		}
	}
	return NULL;
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

void printCandidate(node_t* node){
	cand_t* cand = node->candidates;
	while (cand != NULL && cand->name != NULL){
		printf("Cand: %s has votes: %d\n",cand->name,cand->count);
		cand = cand->next;
	}

}

void construct_count_data(char** data, cand_t* candidates){
	cand_t* cand= candidates;
	if (candidates == NULL || candidates->name == NULL){
		*data = "No votes\0";
	}
	else{
		char* append_data ;
		while (cand != NULL && cand->name != NULL){
			append_data = (char*) malloc(sizeof(char)*256);
			// strcat(*data,cand->name);
			// strcat(*data, ":");
			// strcat(*data, itoa(cand->count));
			snprintf(append_data,256,"%s:%d", cand->name, cand->count);
			strcat(*data,append_data);
			if (cand->next != NULL && cand->next->name != NULL) strcat(*data, ",");
			else strcat(*data,"\0");
			cand = cand->next;
			free(append_data);
			// printf("data: %s\n",*data);
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


void freemakeargv(char **argv) {
   if (argv == NULL)
      return;
   if (*argv != NULL)
      free(*argv);
   free(argv);
}

// take a message and get the region component
// input : AB;SomeRegion           ; ...
// output : SomeRegion
char* getRegionFromMessage(char* message) {
	int m_len = strlen(message);

	char* region_name = (char*) malloc(sizeof(char)*15);
	int i;
	for (i = 3; i < 18; i++) {
		if ( message[i] != ' ' ) {
			region_name[i-3] = message[i];
		} else {
			break;
		}
	}
	region_name[i-3] = '\0';
	return region_name;
}

// take a message and get the data component
// input : AB;SomeRegion           ;A:1,B:2
// output : A:1,B:2
char* getDataFromMessage(char* message) {
	int m_len = strlen(message);

	char* data = (char*) malloc(sizeof(char)*237);
	int i;
	for (i = 19; i < m_len; i++) {
		if ( message[i] != ' ' ) {
			data[i-19] = message[i];
		} else {
			break;
		}
	}
	data[i-19] = '\0';
	return data;
}

// in candidates, increment candidate with name, name, by count, count
void incrementCandidateBy(cand_t* candidates, char* name, int count) {

	if (strcmp(name,"\n")==0) return;

	//adding candidate for first time
	if (candidates->name == NULL){
		candidates->name = (char*) malloc(sizeof(char)*1024);
		strcpy(candidates->name, name);
		candidates->count = 0;
		candidates->count += count;
		return;
	}
	else{
		//check if candiadte has already been added
		cand_t* pointer = candidates;
		cand_t* lastP;
		while(pointer != NULL && pointer->name != NULL){
			if (strcmp(pointer->name, name)==0){
				pointer->count += count;
				return;
			}
			lastP = pointer;
			pointer = pointer->next;
		}
		//if not added, create new node
		cand_t* newCandidate = (cand_t*) malloc(sizeof(cand_t));
		newCandidate->name = (char*) malloc(sizeof(char)*1024);
		strcpy(newCandidate->name, name);
		newCandidate->count = count;
		lastP->next = newCandidate;
		newCandidate->next = NULL;
	}
	return;
}

// in candidates, decrement candidate with name, name, by count, count
int decrementCandidateBy(cand_t* candidates, char* name, int count) {

	if (strcmp(name,"\n")==0) {
		return -11;
	} else {
		//check if candiadte has already been added
		cand_t* pointer = candidates;
		cand_t* lastP;
		while(pointer != NULL && pointer->name != NULL) {
			if (strcmp(pointer->name, name)==0){
				// found candidate
				int new_count = pointer->count - count;
				if ( new_count < 0 ) {
					// decrementation results in negative count
					// don't do anything and signal this is bad
					return -1;
				} else {
					// do decrementation
					pointer->count = new_count;
					return 1;
				}
			}
			lastP = pointer;
			pointer = pointer->next;
		}
		// candidate to remove votes by was not found raise signal
		return -2;
	}
}

int addDataToServerResponse(char* server_response, char* data, int offset) {
	int d_len = strlen(data);
	int j;
	for (j = 0; j < d_len; j++) {
		server_response[j+offset] = data[j];
	}
	server_response[j+offset] = '\0';
	return j + offset;
}

int makeUnhandledCommandServerResponse(char* server_response, char* msg) {
	server_response[0] = 'U';
	server_response[1] = 'C';
	server_response[2] = ';';

	int m_len = strlen(msg);
	int j;
	for (j = 0; j < m_len; j++) {
		if ( msg[j] == ';' || msg[j] == ' ' ) {
			break;
		} else {
			server_response[j+3] = msg[j];
		}
	}
	server_response[j+3] = '\0';
	return j + 3;
}
