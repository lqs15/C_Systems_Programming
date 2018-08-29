/* Authors : Huangying Kuang, Mitchell Luhm */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "makeargv.h"

#define MAX_NODES 100

int NUMBER_CANDIDATES = 0;
char** ALL_CANDIDATES;
int CURRENT_LINE = 1;

//Function signatures

void saveCandidates(char *cand_s);
void saveNodeNames(char *s,node_t *n);
void addChildren(char *s,node_t *n);
int parseInput(char *filename, node_t *n);
int parseInputLine(char *s, node_t *n, int line_num);
void setInitialStatus(node_t *n);
void execNodes(node_t *n);
void forkChildren(node_t *n, int n_children);
int getNodeListLength(node_t *n);
void doubleCheckCycle(node_t *candidate, node_t *parent, node_t *n);
void detectCycle(node_t *n, int i);

/**Function : parseInput
 * Arguments: 'filename' - name of the input file
 * 			  'n' - Pointer to Nodes to be allocated by parsing
 * Output: Number of Total Allocated Nodes
 * About parseInput: parseInput is supposed to
 * 1) Open the Input File [There is a utility function provided in utility handbook]
 * 2) Read it line by line : Ignore the empty lines [There is a utility function provided in utility handbook]
 * 3) Call parseInputLine(..) on each one of these lines
 ..After all lines are parsed(and the DAG created)
 4) Assign node->"prog" ie, the commands that each of the nodes has to execute
 For Leaf Nodes: ./leafcounter <arguments> is the command to be executed.
 Please refer to the utility handbook for more details.
 For Non-Leaf Nodes, that are not the root node(ie, the node which declares the winner):
 ./aggregate_votes <arguments> is the application to be executed. [Refer utility handbook]
 For the Node which declares the winner:
 This gets run only once, after all other nodes are done executing
 It uses: ./find_winner <arguments> [Refer utility handbook]
 */
int parseInput(char *filename, node_t *n){
	FILE* fp = file_open(filename);
	
	char* buf = (char*) malloc(sizeof(char)*1024);
	buf = read_line(buf,fp);

	while (buf != NULL){
		// CURRENT_LINE is line numbering that doesnt count comments as lines	
		if ( buf[0] != '#' && buf != "\n") {
			// error checking if first line not a valid first line
			if (CURRENT_LINE == 1 && (buf[0] < '0' || buf[0] > '9')) {
				printf("First char in first line not a number\n");
				//exit(1);
			} 
			else{
				int p = parseInputLine(buf, n, CURRENT_LINE);
				if (CURRENT_LINE == 2 && p < 2) {
					printf("Only have 0 or 1 nodes, no regions for voting.\n");
					exit(1);
				}
				CURRENT_LINE++; // read current line successfully
			}
		}
		// always get the next line
		buf = read_line(buf, fp);
		
	}	
	setInitialStatus(n);
	free(buf); // KILL BUF MEMORY SPACE
	int node_length = getNodeListLength(n);
	return node_length; 
}
/**Function : parseInputLine
 * Arguments: 's' - Line to be parsed
 * 			  'n' - Pointer to Nodes to be allocated by parsing
 * Output: Number of Region Nodes allocated
 * About parseInputLine: parseInputLine is supposed to
 * 1) Split the Input file [Hint: Use makeargv(..)]
 * 2) Recognize the line containing information of
 * candidates(You can assume this will always be the first line containing data).
 * You may want to store the candidate's information
 * 3) Recognize the line containing "All Nodes"
 * (You can assume this will always be the second line containing data)
 * 4) All the other lines containing data, will show how to connect the nodes together
 * You can choose to do this by having a pointer to other nodes, or in a list etc-
 * */

int parseInputLine(char *s, node_t *n, int line_num){
	if (line_num == 1) {
		saveCandidates(s);
	} else if (line_num == 2){
		saveNodeNames(s, n);
		return getNodeListLength(n);
	} else {
		addChildren(s,n);
	}
	
	return 0; 
}

/* handles line 2 of input.txt          
 *  - Process names of nodes
 *  - Create node list structure */
void saveNodeNames(char *s,node_t *n){
	char** line;
	int length = makeargv(s, " ", &line);
	int i;
	for (i = 0; i < length; i++){
		if(i+1 == length) {
			line[i][strlen(line[i])-1] = 0;
		}

		(n+i)->id = i;
		strcpy((n+i)->name, line[i]);
		char temp[1024] = "";
		strcat(temp,line[i]);
		//strcpy((n+i)->output,strcat(temp,"_output"));			
		prepend(temp, "Output_");		
		strcpy((n+i)->output, temp);
	} 
}

// This is basically the "findnode" function from makeargv.h
node_t* getNodeByName(char* name, node_t* n){
	int j = 0;
	while ( strcmp( (n+j)->name, "" ) != 0 ){
		if ( strcmp( (n+j)->name,  name) == 0 ){
			return (n+j);
		}
		j++;
	}
	printf("Error: function getNodeByName did not find target node.\n");
	return n;
}

void addChildren(char *s,node_t *n){
	// separate parent from the children
	char** parts;
	int nparts = makeargv(s,":", &parts);
	char* parent = trimwhitespace(parts[0]);
	// separate children from the other children
	char** pars_children;
	int nchildren = makeargv(parts[1]," ",&pars_children);

	// get parent node, assign num_children
	node_t *parentNode = getNodeByName(parent,n);
	int parentId = parentNode->id;
	(n+parentId)->num_children = nchildren;		

	int i;
	for (i = 0; i < nchildren; i++){

		// strip newline special character
		if( i+1 == nchildren){
			pars_children[i][strlen(pars_children[i])-1] = 0;
		}
		// get the current child node, tell its parent what the id is
		node_t *childNode = getNodeByName(pars_children[i],n);
		int childId = childNode->id;
		(n+parentId)->children[i] = childId;
	} 
}


// from UNIX Systems Programming Textbook
pid_t r_wait(int *loc){
	int retval;
	while(((retval = wait(loc)) == -1) && (errno = EINTR));
	return retval;
}

/**Function : execNodes
 * Arguments: 'n' - Pointer to Nodes to be allocated by parsing
 * About execNodes: parseInputLine is supposed to
 * If the node passed has children, fork and execute them first
 * Please note that processes which are independent of each other
 * can and should be running in a parallel fashion
 * */
void execNodes(node_t *n) {
	printf("root: %s\n",n->name);
	forkChildren(n, n->num_children);
	printf("FINISHED\n");
	pid_t last_process;
	last_process = fork();
	if (last_process > 0) {
		wait(NULL); // wait for child to exec
		free(n); // KILL THE NODES
		free(ALL_CANDIDATES); // KILL THE CANDIDATES
	} else if (last_process == 0) {
		execv(n->prog, n->arguments);
		perror("Exec failed\n");
		exit(1);
	} else {
		perror("Fork failed;");
		exit(1);
	}
}


void forkChildren(node_t *n, int n_children){
	int i;
	pid_t pid;
	for (i=0; i< n_children; i++){
		pid = fork();

		node_t* childNode = findNodeByID(n,n->children[i]);
		if (pid == 0) {
			if (childNode->num_children>0) {
				forkChildren(childNode, childNode->num_children);
			}
			execv(childNode->prog, childNode->arguments);
			perror("Execv failed\n");
			exit(1);
		}
		else if (pid > 0) {

			int status = 0;
			pid_t wpid;
			while ((wpid = wait(&status)) > 0);
			
			printf("\nabove belong to parent: %s\n", childNode->name);
		} else if (pid == -1) {
			perror("Fork Failed\n");
			exit(1);
		}
		
	}

}


void saveCandidates(char *cand_s) {
	char **line;
	int split_length = makeargv(cand_s, " ", &line);
	
	NUMBER_CANDIDATES = atoi(line[0]);

	ALL_CANDIDATES = (char**) malloc(sizeof(char*) * (split_length - 1));

	// copy stuff from line to ALL_CANDIDATES global variable
	for (int i = 1; i < split_length; i++) {
		if (i + 1 == split_length) {
			line[i][strlen(line[i])-1] = 0;
		} 
		ALL_CANDIDATES[i-1] = line[i];
	}
}


// go through all the nodes, set the arguments for exec based on node type
void setInitialStatus(node_t *n) {
	int i;
	for (i = 0; strcmp((n+i)->name, "") != 0; i++) {

		int F_INDEX;

		if ( strcmp( (n+i)->name, "Who_Won") == 0 ) {
			// is root node 
			(n+i)->status = 1; 
			int r_num_child = (n+i)->num_children;
			char* str;
			asprintf (&str, "%i", r_num_child);
			strcpy( (n+i)->prog, "./find_winner");
			(n+i)->arguments[0] = "root";
			(n+i)->arguments[1] = str; // number of children
			int rc;
			for (rc = 2; rc < r_num_child+2; rc++) {
				char* out = (findNodeByID(n, (n+i)->children[rc-2]))->output;
				(n+i)->arguments[rc] = out;
			}
			(n+i)->arguments[rc] = (n+i)->output;
			F_INDEX = rc+1;
		} else if ( (n+i)->num_children == 0 ) {
			// is leaf node
			(n+i)->status = 13; 
			strcpy( (n+i)->prog, "./leafcounter");
			(n+i)->arguments[0] = "leaf";
			(n+i)->arguments[1] = (n+i)->name;
			(n+i)->arguments[2] = (n+i)->output;
			F_INDEX = 3;
		} else {
			// is not leaf node and not root node (middle node)
			(n+i)->status = 2; 
			int m_num_child = (n+i)->num_children;
			char* str;
			asprintf (&str, "%i", m_num_child);
			strcpy( (n+i)->prog, "./aggregate_votes");
			(n+i)->arguments[0] = "middle_node";
			(n+i)->arguments[1] = str; // number of children
			int mc;
			for (mc = 2; mc < m_num_child+2; mc++) {
				char* out = (findNodeByID(n, (n+i)->children[mc-2]))->output;
				(n+i)->arguments[mc] = out;
			}
			(n+i)->arguments[mc] = (n+i)->output;
			F_INDEX = mc+1;
		} 
		char* str;
		asprintf (&str, "%i", NUMBER_CANDIDATES);
		(n+i)->arguments[F_INDEX] = str;
		F_INDEX++;
		int j;
		for (j=0; j<NUMBER_CANDIDATES; j++){
			(n+i)->arguments[F_INDEX+j] = ALL_CANDIDATES[j];
		}				
		(n+i)->arguments[F_INDEX+j] = NULL;
		
		/* NODE PARAM DEBUGGING PRINTS ************* 
		printf(" - - - - - - - - - - - \n"); 
		printf("NODE NAME: %s\n", (n+i)->name); 
		printf("PROG NAME: %s\n", (n+i)->prog);
		for (int a = 0; a < F_INDEX+j+1; a++) 
			printf("ARG: %s\n", (n+i)->arguments[a]);
		*/
	}	
}

// get length of a node list
int getNodeListLength(node_t *n) {
	int i = 0;
	while (strcmp((n+i)->name, "") != 0) i++;
	return i;
}

void doubleCheckCycle(node_t *candidate, node_t *parent, node_t *n){
	int i;
	for (i = 0 ; i < candidate->num_children; i++){
		if (candidate->children[i] == parent->id){
			printf("CYCLE FOUND!\n");
			printf("Node with name %s causes a cycle\n", parent->name);
			printf("Exiting Program\n");
			exit(1);	
		}
	}
	for (i = 0 ; i < candidate->num_children; i++){
		node_t *nextNode = findNodeByID(n, candidate->children[i]);
		doubleCheckCycle(nextNode, parent, n);
	}
}

void detectCycle(node_t *n, int i) {
	(n+i)->status = 1; // we have visisted this node
	for (int j = 0; j < (n+i)->num_children; j++) {
		if ( (findNodeByID(n, ((n+i)->children[j])))->status == 1 ) {
			// this child has been visited already
			doubleCheckCycle( findNodeByID(n, ((n+i)->children[j])), (n+i), n );
		}
		detectCycle(n, (n+i)->children[j]);	
	}
}

int main(int argc, char **argv){

	//Allocate space for MAX_NODES to node pointer
	struct node* mainnodes=(struct node*)malloc(sizeof(struct node)*MAX_NODES);

	if (argc != 2 && argc != 3){
		printf("Usage: %s Program\n", argv[0]);
		printf("Expected one program argument: an input.txt file, ");
		printf("but the progam was given %d arguments.\n", argc-1);
		return -1;
	}

	//call parseInput
	int num = parseInput(argv[1], mainnodes);

	if (argc > 2 && strcmp(argv[2], "-circle") == 0) {
		detectCycle(mainnodes, 0);
	}
	//Call execNodes on the root node
	printf("start executing!!---------------------->\n");
	execNodes(mainnodes);

	return 0;
}
