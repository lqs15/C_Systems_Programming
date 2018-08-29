/* file : votecounter.c */

/*login: kuang048, luhmx009  <--- UMN x500s
* date: 04/11/18
* name: Huangying Kuang, Mitchell Luhm
* id: kuang048, luhmx009
* extra credit NOT implemented */

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
#include "makeargv.h"
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>

/* The maximum amount of bytes for a file name */
#define MAX_FILE_NAME_SIZE 255

/* The maximum amount of bytes for each I/O operation */
#define MAX_IO_BUFFER_SIZE 1024

//Global value
pthread_mutex_t* log_lock;
char* log_file;

//Function signatures
int parseInput(char *filename, node_t *n);
node_t * getNodeByName(char* name, node_t* n);

/** Parse the input of the input DAG file
 *
 * Assume that first line is root.
 * Always create nodes for all entries specified on a line.
 */
int parseInput(char *filename, node_t *n){
	FILE* fp = file_open(filename);

	char* buf = (char*) malloc(sizeof(char)*1024);
	buf = read_line(buf,fp);
	int i = 0;

	while (buf != NULL) {
		char** line = (char**) malloc(sizeof(char*) * 1024);
		int length = makeargv(buf, ":", &line);
 		int ls;

		node_t * current_node; // = (node_t*) malloc(sizeof(node_t*));
		if ( i == 0 ) {
			current_node = n;
			current_node->name = (char*) malloc(sizeof(char)*1024);
			strcpy(current_node->name, line[0]);
			current_node->node_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(current_node->node_lock, NULL);
			current_node->status = 0;
			current_node->candidates = (cand_t*) malloc(sizeof(cand_t));
			current_node->num_times_written_to = 0;
		}
		else {
			current_node = getNodeByName(line[0], n);
		}

		current_node->num_children = length - 1;
		current_node->result_file_created = 0;

		current_node->children = (node_t **) malloc(sizeof(node_t *) * (current_node->num_children));
		// WE CAN MAKE THE CHILDREN NODES HERE,
		// WHEN WE READ LINE FOR CHILDREN LATER, CHECK IF IT WAS ALREADY MADE
		for (int c = 0; c < (current_node->num_children); c++) {
			node_t * new_child_node = (node_t *) malloc(sizeof(node_t));
			new_child_node->name = (char*) malloc(sizeof(char)*1024);
			new_child_node->node_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(new_child_node->node_lock, NULL);
			new_child_node->parent = current_node;
			new_child_node->status = 0;
			new_child_node->candidates = (cand_t*) malloc(sizeof(cand_t));
			strcpy(new_child_node->name, line[c + 1]);

			// set defaults
			new_child_node->num_children = 0;
			new_child_node->result_file_created = 0;
			new_child_node->num_times_written_to = 0;
			int name_length = strlen(new_child_node->name);
			if (c == current_node->num_children - 1) {
				if ( (new_child_node->name)[name_length-1] == '\n' ) {
					// strip the \n out
					(new_child_node->name)[name_length-1] = '\0';
				}
			}
			(current_node->children)[c] = new_child_node;
		}
	    buf = read_line(buf, fp);
	    i++;
	}
	free(buf); // KILL BUF MEMORY SPACE
	return 1;
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

void create_output_dir(node_t* root, char* out_dir, int call_num){
	if ( call_num == 1 ) {
		char* rootname = root->name;
		root->path_to_node = makeFilePath(out_dir,rootname);
	}
	else{
		root->path_to_node = makeFilePath(root->parent->path_to_node,root->name);
	}
	mkdir(root->path_to_node,0777);
	chmod(root->path_to_node,0700);
	int i;
	for (i = 0; i < root->num_children; i ++){
		create_output_dir((root->children)[i], out_dir, 2);
	}
}

void create_log_file(char* output_dir){
	log_file = makeFilePath(output_dir,"log.txt");
	FILE *f = fopen(log_file, "w");
	if (f == NULL)
	{
		perror("create_log_file(): error opening file\n");
		exit(1);
	}
	log_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(log_lock, NULL);
	fclose(f);
}

void check_if_all_input_empty(node_t* root, char* input_dir) {
	DIR *dir;
	struct dirent *file;
	int non_empty_files = 0;
	if ( (dir = opendir(input_dir)) != NULL ) {
		while ( (file = readdir(dir)) != NULL ) {
			printf("in check_if\n");
			// see if file->d_name empty
			// check if file is empty
			if (file->d_name[0] != '.') {
				struct stat f_stat;
				printf("makeFIlePath %s", makeFilePath(input_dir, file->d_name));
			
				if (stat(makeFilePath(input_dir, file->d_name), &f_stat) != 0) {
						printf("first if\n");
				}

				if (f_stat.st_size != 0) {
					// File is not blank
					printf("second if\n");
					non_empty_files++;
				}
			}
		}
		closedir(dir);
		if (non_empty_files == 0) {
			perror("all input files are empty. Aborting.\n");
			exit(1);
		}
	}
}

/** Get number of leaves a node graph has
 * INPUT  : Root node
 *          path to input directory
 * OUTPUT : Number of leaf nodes
 */
int get_number_of_leaves(node_t* root, char* input_dir) {
	DIR *dir;
	struct dirent *file;
	int leaf_file_count = 0;
	if ( (dir = opendir(input_dir)) != NULL ) {
		while ( (file = readdir(dir)) != NULL ) {
			node_t* cur_node = getNodeByName(file->d_name, root);
			if ( cur_node != NULL && cur_node->num_children == 0) {
				leaf_file_count++;
			}
		}
		closedir(dir);
	}
	return leaf_file_count;
}

/** Create and return a leaf_queue structure
 * INPUT  : Root node
 *          path to input directory
 * OUTPUT : leaf_queue structure pointer
 *            (see makeargv.h for stuct details)
 */
leaf_q* create_leaf_queue(node_t* root, char* input_dir) {
	leaf_q* q = (leaf_q*) malloc(sizeof(leaf_q));
	DIR *dir;
	struct dirent *file;
	int n_leaves = get_number_of_leaves(root, input_dir);
	if (n_leaves == 0) {
		// input directory is completely empty
		perror("error: input directory is empty\n");
		exit(1);
	}
	q->num_leaves = n_leaves;
	q->leaf_nodes = (node_t**) malloc(sizeof(node_t*)*n_leaves);
	q->q_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(q->q_lock, NULL);
	q->retrieved = (int*) malloc(sizeof(int)*n_leaves);
	int i = 0;

	if ( (dir = opendir(input_dir)) != NULL ) {
		while ( (file = readdir(dir)) != NULL ) {
			node_t* current_node = getNodeByName(file->d_name, root);
			if ( current_node != NULL && current_node->num_children == 0) {
				(q->leaf_nodes)[i] = current_node;
				(q->retrieved)[i] = 0;
				i++;
			}
		}
		closedir(dir);
	}
	return q;
}

/*
 * find node in list if exists
 * - if it doesn't exist, add it
 * increment its count attribute
 */

void incrementCandidate(cand_t* candidates, char* name, pthread_mutex_t* node_lock){

	if (strcmp(name,"\n")==0) return;

	//adding candidate for first time
	if (candidates->name == NULL){
		candidates->name = (char*) malloc(sizeof(char)*1024);
		strcpy(candidates->name, name);
		candidates->count = 0;
		candidates->count ++;
		return;
	}
	else{
		//check if candiadte has already been added
		cand_t* pointer = candidates;
		cand_t* lastP;
		while(pointer != NULL && pointer->name != NULL){
			if (strcmp(pointer->name, name)==0){
				pointer->count ++;
				return;
			}
			lastP = pointer;
			pointer = pointer->next;
		}
		//if not added, create new node
		cand_t* newCandidate = (cand_t*) malloc(sizeof(cand_t));
		newCandidate->name = (char*) malloc(sizeof(char)*1024);
		strcpy(newCandidate->name, name);
		newCandidate->count = 1;
		lastP->next = newCandidate;
		newCandidate->next = NULL;
	}
	return;
}


/*
 * takes the filePath to read a votes.txt
 * creates the node structure of candidate names and their
 * vote counts. This node structure is passed to
 * makeOutputFile(), to produce the output text files for
 * a given leaf.
 */

void readDecryptedFile(char* filePath, char* path, node_t* leaf_node) {
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
	for (i = 0; i < length; i++){
		incrementCandidate(leaf_node->candidates, line[i],leaf_node->node_lock);
	}

	if (i == 0) {
		printf("There was an empty votes.txt file\n");
	}

	freemakeargv(line); //not sure about this function from util.h
}


void decryptAndStore(node_t* leaf_node) {
	char* filePath = makeFilePath(leaf_node->input_path, leaf_node->name);
	int fileread = open(filePath,O_RDWR);
	if (fileread == -1) {
		perror("Leaf_Counter:readVotes, error opening file\n");
		exit(1);
	}

	// check if file is empty
	struct stat f_stat;
	if (stat(filePath, &f_stat) != 0) {
		close(fileread);
		return;
	}

	if (f_stat.st_size == 0) {
		// File is blank, dont' do anything
		close(fileread);
		return;
	}

	char* buf = (char*) malloc(sizeof(char)*MAX_IO_BUFFER_SIZE);
	int nread = read(fileread,buf,MAX_IO_BUFFER_SIZE);
	if (nread == -1) {
		perror("Leaf_Counter:readVotes, error reading file\n");
		exit(1);
	}

	// open file for writing
	char* filePathWrite = makeFilePath(leaf_node->path_to_node, leaf_node->name);
	strcat(filePathWrite, ".txt");
	FILE *write_file = fopen(filePathWrite, "w");
	if (write_file == NULL) {
		perror("Can't make decrypted file\n");
		exit(1);
	}

	//divide file into lines
	char** line;
	int length = makeargv(buf, "\n", &line);
	int i;
	node_t* candidates = (node_t*) malloc(sizeof(node_t));
	if (f_stat.st_size == 2) {
		// there is only one line in the file, for some reason makeargv gives this a length of 2
		length = 1;
	}


	for (i = 0; i < length; i++){
		if (i > 0) { fprintf(write_file, "\n"); }
		int line_length = strlen(line[i]);
		int j;
		for (j = 0; j < line_length; j++) {

			char raw_ch = line[i][j];
			char new_ch;
			if ( ('a' <= raw_ch && raw_ch <= 'x') ||
				 ('A' <= raw_ch && raw_ch <= 'X') ) {
				new_ch = raw_ch + 2;
			} else if ('y' == raw_ch || 'Y' == raw_ch ||
					   'z' == raw_ch || 'Z' == raw_ch) {
				new_ch = raw_ch - 24;
			} else {
				new_ch = raw_ch;
			}
			fprintf(write_file, "%c", new_ch);
		}
	}

	fclose(write_file);
	close(fileread);

	if (i == 0) {
		printf("There was an empty votes.txt file\n");
	}
	freemakeargv(line); //not sure about this function from util.h
	readDecryptedFile(filePathWrite, filePathWrite, leaf_node);
}

// result file was already create it, aggregate it using info from leaf_node->candidates
void appendResultFile(node_t* leaf_node, node_t* ancestor_node, pthread_mutex_t* node_lock) {
	pthread_mutex_lock(node_lock);
	node_t* cur_ancestor = ancestor_node;
	cand_t* leaf_cand = leaf_node->candidates;

	int i = 0;
	while (leaf_cand != NULL) {
		int j;
		for (j = 0; j < leaf_cand->count; j++) {
			incrementCandidate(cur_ancestor->candidates, leaf_cand->name, cur_ancestor->node_lock);
		}
		leaf_cand = leaf_cand->next;
		i++;
	}

	char* filePathWrite = makeFilePath(cur_ancestor->path_to_node, cur_ancestor->name);
	strcat(filePathWrite, ".txt");

	FILE *write_file = fopen(filePathWrite, "w");
	if (write_file == NULL) {
		perror("can't make file\n");
		exit(1);
	}

	cand_t* ancestor_cand = cur_ancestor->candidates;

	i = 0;
	while (ancestor_cand != NULL) {
		if (i > 0) { fprintf(write_file, "\n"); }
		fprintf(write_file, "%s:%d", ancestor_cand->name, ancestor_cand->count);
		ancestor_cand = ancestor_cand->next;
		i++;
	}

	fclose(write_file);
	ancestor_node->num_times_written_to ++;
	pthread_mutex_unlock(node_lock);
}

/* create the result file, write to it the information from leaf_node->candidates
* update result_file_created to 1
*/
void createResultFile(node_t* leaf_node, node_t* ancestor_node, pthread_mutex_t* node_lock) {
	pthread_mutex_lock(node_lock);
	ancestor_node->result_file_created = 1;
	node_t* cur_ancestor = ancestor_node;

	char* filePathWrite = makeFilePath(cur_ancestor->path_to_node, cur_ancestor->name);
	strcat(filePathWrite, ".txt");

	FILE *write_file = fopen(filePathWrite, "w");
	if (write_file == NULL) {
		perror("can't make file\n");
		exit(1);
	}

	cand_t* cur_cand = leaf_node->candidates;

	int i = 0;
	while (cur_cand != NULL) {
		if (i > 0) { fprintf(write_file, "\n"); }
		fprintf(write_file, "%s:%d", cur_cand->name, cur_cand->count);

		// increment parent's candidates
		int j;
		for (j = 0; j < cur_cand->count; j++) {
			incrementCandidate(cur_ancestor->candidates, cur_cand->name, node_lock);
		}
		cur_cand = cur_cand->next;
		i++;
	}

	fclose(write_file);
	cur_ancestor->num_times_written_to ++;
	pthread_mutex_unlock(node_lock);
}

/**
 * - Check if parent has a result file,
 *   	- call createResultFile(...) if it doesn't have one
 *   	- call appendResultFile(...) if it has one
 */
void writeResultFile(node_t* leaf_node, node_t* ancestor_node) {
	if (leaf_node->parent != NULL && ancestor_node != NULL) {
		// has parent and ancestor exists
		if (ancestor_node->result_file_created == 0) {
			createResultFile(leaf_node, ancestor_node, ancestor_node->node_lock);
		} else {
			appendResultFile(leaf_node, ancestor_node, ancestor_node->node_lock);
		}
	}
}

void *countTask(void* arg) {
	node_t* leaf_node = arg;
	leaf_node->status = 1;
	writeLog(leaf_node->name,(long)pthread_self(),"start",log_file, log_lock);
	decryptAndStore(leaf_node);
	writeResultFile(leaf_node, leaf_node->parent);

	// NOW TRAVESE UP DAG AND WRITE RESULT FILES FOR "PARENTS OF PARENTS..."
	if (leaf_node->parent != NULL) {
		node_t* cur_ancestor = leaf_node->parent->parent;
		while (cur_ancestor != NULL) {
			writeResultFile(leaf_node, cur_ancestor);
			cur_ancestor = cur_ancestor->parent;
		}
	}
	writeLog(leaf_node->name,(long)pthread_self(),"end",log_file, log_lock);
	return NULL;
}

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
  int length = makeargv(buf, "\n", &candidates);
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

int main(int argc, char **argv){

  node_t * root_node = (node_t *) malloc(sizeof(node_t));

	if (argc != 4 ){
		printf("Expected three program argument: A file that contains the graph, Path of the input directory, and Path of the output directory  ");
		printf("but the progam was given %d arguments.\n", argc-1);
		return -1;
	}

	char* input_filename = argv[1];
	char* input_dir = argv[2];
	char* output_dir = argv[3];

	//call parseInput
	int num = parseInput(argv[1], root_node);
	//printgraph(root_node);

	mkdir(output_dir,0777);
	chmod(output_dir,0700);
	create_output_dir(root_node, output_dir, 1);
	create_log_file(output_dir);

	leaf_q* queue = create_leaf_queue(root_node, input_dir);

	check_if_all_input_empty(root_node, input_dir);

	char* final_root_output = (char*) malloc(sizeof(char)*4096);
	strcpy(final_root_output, root_node->path_to_node);
	strcat(final_root_output, "/");
	strcat(final_root_output, root_node->name);
	strcat(final_root_output, ".txt");
	printf("final_root_output %s\n", final_root_output);

	pthread_t threads[queue->num_leaves];
	for (int i = 0; i <  queue->num_leaves; i++){
		node_t* leaf_node = (queue->leaf_nodes)[i];
		leaf_node->input_path = (char*) malloc(sizeof(char)*1024);
		strcpy(leaf_node->input_path, input_dir);
		// write log.txt
		pthread_create(&threads[i], NULL, countTask, (void*) leaf_node);

	}
	for (int i = 0; i < queue->num_leaves; i++) { pthread_join(threads[i], NULL);}

	// append WINNER: ....
	findWinner(final_root_output);
	return 0;
}
