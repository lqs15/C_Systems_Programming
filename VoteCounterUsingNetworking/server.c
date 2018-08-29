/* file : server.c */

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
#include "makeargv.h"
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* The maximum amount of bytes for a candidate name */
#define CANDIDATE_NAME 100

/* The maximum amount of bytes for region name */
#define MAX_REGION_NAME 15

/* The maximum amount of bytes for a message client sends */
#define MAX_MESSAGE_SIZE 256

#define NUM_ARGS 2

// root node global access
node_t * root_node;

typedef struct thread_arguments {
  int sock_id;
  char *ip;
  uint16_t port;
} thread_arg;


/* client NO. */
int client_num = 0;

/* global lock*/
pthread_mutex_t* node_lock;

//Function signatures
int parseInput(char *filename, node_t *n);
node_t * getNodeByName(char* name, node_t* n);

int parseInput(char *filename, node_t *n){
	FILE* fp = file_open(filename);

	char* buf = (char*) malloc(sizeof(char)*1024);
	buf = read_line(buf,fp);
	int i = 0;

	while (buf != NULL) {
		char** line = (char**) malloc(sizeof(char*) * 1024);
		int length = makeargv(buf, ":", &line);
 		int ls;

		node_t * current_node;
		if ( i == 0 ) {
			current_node = n;
			current_node->name = (char*) malloc(sizeof(char)*MAX_REGION_NAME);
			strcpy(current_node->name, line[0]);
			current_node->status = 0;
			current_node->candidates = (cand_t*) malloc(sizeof(cand_t));
			current_node->num_times_written_to = 0;
			current_node->is_polling_open = 0;
		}
		else {
			current_node = getNodeByName(line[0], n);
		}

		current_node->num_children = length - 1;
		current_node->children = (node_t **) malloc(sizeof(node_t *) * (current_node->num_children));
		// WE CAN MAKE THE CHILDREN NODES HERE,
		// WHEN WE READ LINE FOR CHILDREN LATER, CHECK IF IT WAS ALREADY MADE
		for (int c = 0; c < (current_node->num_children); c++) {
			node_t * new_child_node = (node_t *) malloc(sizeof(node_t));
			new_child_node->name = (char*) malloc(sizeof(char)*MAX_REGION_NAME);
			new_child_node->parent = current_node;
			new_child_node->status = 0;
			new_child_node->candidates = (cand_t*) malloc(sizeof(cand_t));
			strcpy(new_child_node->name, line[c + 1]);

			// set defaults
			new_child_node->num_children = 0;
			new_child_node->num_times_written_to = 0;
			new_child_node->is_polling_open = 0;
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


int create_server_socket(int port){
  int sock = socket(AF_INET , SOCK_STREAM , 0);

  // Bind it to a local address.
  struct sockaddr_in servAddress;
  servAddress.sin_family = AF_INET;
  servAddress.sin_port = htons(port);
  servAddress.sin_addr.s_addr = htonl(INADDR_ANY);

  if( bind(sock, (struct sockaddr *) &servAddress, sizeof(servAddress)) < 0){
    perror("bind failed. Error");
    //return 1;
  }

  if ( listen(sock, 4) < 0){
    perror("listen failed. Error");
    //return 1;
  }
  printf("Server listening on port %d\n", port);
  return sock;
}




void aggregate_leaf_cands(node_t* node, cand_t* aggregation_cands) {
  int n_children = node->num_children;
  //not leaf node, keep going down the tree
  if (node->num_children > 0) {
    int i;
    for (i = 0 ; i < n_children; i++) {
      aggregate_leaf_cands((node->children)[i], aggregation_cands);
    }
  }
  //leaf node, add this node's candidates votes to aggregating candidates
  else{
    cand_t* cand = node->candidates;
  	while (cand != NULL && cand->name != NULL){
  		incrementCandidateBy(aggregation_cands, cand->name, cand->count);
  		cand = cand->next;
  	}
  }
}


/* INPUT FROM CLIENT : a region
 * RETURN : candidate with majority of votes in specified region (including
 *          sub-regions of specifief region)
 * REQUIRES : nothing (polls can be closed or open)
 * EXCEPTION : if region not opened yet, return "No votes."
 */
char* count_votes(char* region_name, char** code) {

  node_t* node = getNodeByName(region_name,root_node);

  if (node == NULL) {
    *code = "NR";
    return "";
  }

  *code = "SC";
  char* data = (char*) malloc(sizeof(char)*256);

  cand_t* counting_candidates = (cand_t*) malloc(sizeof(cand_t));
  aggregate_leaf_cands(node,counting_candidates);



  construct_count_data(&data, counting_candidates);
  if (strcmp(region_name,root_node->name) == 0) root_node->candidates = counting_candidates;


  if (node->candidates )
  return(data);
}

/* INPUT FROM CLIENT : nothing
 * RETURN : candidate with majority of votes over all areas
 * REQUIRES : that every region's polls have been opened and closed
 */
char* return_winner(char** code) {
  //check if all polls have been opened and closed
  if (root_node->is_polling_open != -1) {
    *code = "RO";
    return("");
  }
  else{
    //aggregate final votes
    *code = "SC";
    count_votes(root_node->name,code);
    //find winner
    int max_votes = -1;
    cand_t* max_candidate = root_node->candidates;
    cand_t* cur_candidate = root_node->candidates;

    while (cur_candidate!=NULL && cur_candidate->name != NULL){
      // if this candidate has more votes than current max,
      // set them as the new max_candidate
      if (cur_candidate->count >= max_votes) {
        max_votes = cur_candidate->count;
        max_candidate = cur_candidate;
      }
      cur_candidate = cur_candidate->next;
    }
    char* return_data = (char*) malloc(sizeof(char)*256);
    if (max_candidate->name == NULL){
  		return_data = "No votes\0";
  	}
  	else{
      return_data[0] = '\0';
    	strcat(return_data, "Winner:");
    	strcat(return_data, max_candidate->name);
    }
    return(return_data);
  }
}

/* Helper recursive function*/
void open_polls_children(node_t* node) {
  if (node->is_polling_open == 0) {
    node->is_polling_open = 1;
  }
  int n_children = node->num_children;
  int i;
  for (i = 0 ; i < n_children; i++) {
    open_polls_children((node->children)[i]);
  }
}

/* INPUT FROM CLIENT : a region
 * PURPOSE : open region for voting
 * RETURN : Respond with PF, Poll Failure, if already open
 * REQUIRES : this must be called before changing votes or closing poll
 */
void open_polls(char* region_name,char** code) {

  node_t* node = getNodeByName(region_name,root_node);
  if (node == NULL) *code = "NR";
  else if (node->is_polling_open == 0) {
    node->is_polling_open = 1;
    *code = "SC";
    //propagate down to children
    int n_children = node->num_children;
  	int i;
  	for (i = 0 ; i < n_children; i++) {
      open_polls_children((node->children)[i]);
    }
  }
  else if (node->is_polling_open == 1) *code = "PF";
  else if (node->is_polling_open == -1) *code = "RR";
  else {
    *code = "UE";
  }
}

/* Helper recursive function*/
void close_polls_children(node_t* node) {
  if (node->is_polling_open == 1) {
    node->is_polling_open = -1;
  }
  int n_children = node->num_children;
  int i;
  for (i = 0 ; i < n_children; i++) {
    close_polls_children((node->children)[i]);
  }
}

/* INPUT FROM CLIENT : a region
 * RETURN : Respond with PF, poll failure, if closed before calling
 * REQUIRES : that region was open before calling
 */
int close_polls(char* region_name,char** code) {
  node_t* node = getNodeByName(region_name,root_node);
  if (node == NULL) {
    *code = "NR";
  } else if (node->is_polling_open == 1) {
    node->is_polling_open = -1;
    *code = "SC";
    //close children polls
    int n_children = node->num_children;
    int i;
    for (i = 0 ; i < n_children; i++) {
      close_polls_children((node->children)[i]);
    }
  } else if (node->is_polling_open == -1) {
    // closing a poll that has already been closed
    *code = "PF";
	return 1;
  } else if (node->is_polling_open == 0) {
    // closing a poll that has never been opened
    *code = "PF";
	return 2;
  } else {
    *code = "UE";
  }
  return 0;
}

/* INPUT FROM CLIENT : a region AND voting data
 * PURPOSE : add votes that correspond to voting data to region specified.
 *           remember to add new candidates as needed if they appear in input
 *           data.
 * REQUIRES : polls must have been opened for region, but not closed yet
 */

void add_votes(char* region, char* data, char** code) {
	// add voting data to region

	// get the region's node
	node_t* region_node = getNodeByName(region, root_node);

  if (region_node == NULL) *code = "NR";
  else if (region_node->num_children > 0) *code = "NL";
  else if (region_node->is_polling_open != 1) *code = "RC";
	else if ( region_node->is_polling_open == 1 ) {
		// increment votes for region_node
		// split data by ,
		char** cands_with_vote;
		int n_cands_to_inc = makeargv(data, ",", &cands_with_vote);
		int i;
		for (i = 0; i < n_cands_to_inc; i++) {

			// split into "CAND, VOTES"
			char** one_cand_with_vote;
			int len = makeargv(cands_with_vote[i], ":", &one_cand_with_vote);

			// candidate name
			char* candidate_name = one_cand_with_vote[0];
			// vote amount as string
			char* vote_number = one_cand_with_vote[1];
			// vote amount as int
			int votes = atoi(vote_number);

			incrementCandidateBy(region_node->candidates, candidate_name, votes);
      *code = "SC";
		}
  }
  else {
    *code = "UE";
  }
}

/* INPUT FROM CLIENT : a region AND voting data
 * PURPOSE : remove votes for a specified region.
 * REQUIRES : polls must have been opened for region, but not closed yet
 * EXCEPTION : remove a candidate that does not exist results in error.
 *             remove more votes than were present before results in error.
 */
char* remove_votes(char* region_name, char* data, char** code) {
	node_t* region_node = getNodeByName(region_name, root_node);
	char* bad_candidate = (char*) malloc(sizeof(char)*254);
	int illegal_subtraction_happened = 0;
	if (region_node == NULL) {
		*code = "NR";
	} else if (region_node->num_children > 0) {
		*code = "NL";
	} else if (region_node->is_polling_open != 1) {
		*code = "RC";
	} else if (region_node->is_polling_open == 1) {
		// split data by ,
		char** cands_with_vote;
		int n_cands_to_inc = makeargv(data, ",", &cands_with_vote);
		int i;
		for (i = 0; i < n_cands_to_inc; i++) {

			// split into "CAND, VOTES"
			char** one_cand_with_vote;
			int len = makeargv(cands_with_vote[i], ":", &one_cand_with_vote);

			// candidate name
			char* candidate_name = one_cand_with_vote[0];
			// vote amount as string
			char* vote_number = one_cand_with_vote[1];
			// vote amount as int
			int votes = atoi(vote_number);

			int dc = decrementCandidateBy(region_node->candidates, candidate_name, votes);
			if ( dc < 0 ) {
				// illegal subtraction;
				// TODO pass this : name is candidate_name;
				if ( illegal_subtraction_happened ) {
					// add , too
					strcat(bad_candidate, ",");
					strcat(bad_candidate, candidate_name);
				} else {
					illegal_subtraction_happened = 1;
					bad_candidate[0] = '\0';
					strcat(bad_candidate, candidate_name);
					//strcpy(bad_candidate, candidate_name);
				}
				*code = "IS";
			} else if ( dc && illegal_subtraction_happened == 0 ) {
				*code = "SC";
			}
		}
		// success in adding candidates
	} else {
		*code = "UE";
	}
	return bad_candidate;
 }

char* handleMessage(char* message) {
	// first two chars are the request code ALWAYS
	char RC_A = message[0];
	char RC_B = message[1];
	char* server_response = (char*) malloc(sizeof(char)*MAX_MESSAGE_SIZE);

  char* code =  (char*) malloc(sizeof(char)*2);
	if ( RC_A == 'R' && RC_B == 'W' ) {
		// Return Winner
    	char* v_data = return_winner(&code);
		if ( code[0] == 'S' && code[1] == 'C' ) {
			addDataToServerResponse(server_response, v_data, 3);
		} else if ( code[0] == 'R' && code[1] == 'O' ) {
			char* region = root_node->name;
			addDataToServerResponse(server_response, region, 3);
		}

	} else if ( RC_A == 'C' && RC_B == 'V' ) {
		// Count Votes
		char* region = getRegionFromMessage(message);
    	char* v_data = count_votes(region, &code);
		if ( code[0] == 'S' && code[1] == 'C' ) {
			addDataToServerResponse(server_response, v_data, 3);
		} else if ( code[0] == 'N' && code[1] == 'R' ) {
			addDataToServerResponse(server_response, region, 3);
		}

	} else if ( RC_A == 'O' && RC_B == 'P' ) {
		// Open Polls
		char* region = getRegionFromMessage(message);
		open_polls(region, &code);

		if ( (code[0] == 'N' || code[0] == 'R') && code[1] == 'R' ) {
			// if code is NR or RR, carry region data into server_response
			addDataToServerResponse(server_response, region, 3);
		} else if ( code[0] == 'P' && code[1] == 'F' ) {
			int j = addDataToServerResponse(server_response, region, 3);
			server_response[j] = ' ';
			server_response[j+1] = 'o';
			server_response[j+2] = 'p';
			server_response[j+3] = 'e';
			server_response[j+4] = 'n';
			server_response[j+5] = '\0';
		}

	} else if ( RC_A == 'A' && RC_B == 'V' ) {
		// Add Votes
		char* region = getRegionFromMessage(message);
		char* data = getDataFromMessage(message);
		add_votes(region, data, &code);
		// if code is NL or RC, carry data with server_response
		if ( (code[0] == 'N' && code[1] == 'L') ||
			 (code[0] == 'R' && code[1] == 'C') ||
		     (code[0] == 'N' && code[1] == 'R') ) {
			addDataToServerResponse(server_response, region, 3);
		}

	} else if ( RC_A == 'R' && RC_B == 'V' ) {
		// Remove Votes
		char* region = getRegionFromMessage(message);
		char* data = getDataFromMessage(message);
		// remove_votes returns bad candidate name if there was an IS.
		char* bad_candidate = remove_votes(region, data, &code);
		if ( code[0] == 'I' && code[1] == 'S' ) {
			// illegal subtraction; include candidate name in response
			addDataToServerResponse(server_response, bad_candidate, 3);
		} else if ( (code[0] == 'N' && code[1] == 'L') ||
			        (code[0] == 'R' && code[1] == 'C') ||
                    (code[0] == 'N' && code[1] == 'R') ) {
			addDataToServerResponse(server_response, region, 3);
		}

	} else if ( RC_A == 'C' && RC_B == 'P' ) {
		// Close Polls
		char* region = getRegionFromMessage(message);
    	int cp = close_polls(region, &code);
		if ( code[0] == 'N' && code[0] == 'R' ) {
			// if code is NR, carry region data into server_response
			addDataToServerResponse(server_response, region, 3);
		} else if ( code[0] == 'P' && code[1] == 'F' ) {
			int j = addDataToServerResponse(server_response, region, 3);
			if ( cp == 1) {
				server_response[j] = ' ';
				server_response[j+1] = 'c';
				server_response[j+2] = 'l';
				server_response[j+3] = 'o';
				server_response[j+4] = 's';
				server_response[j+5] = 'e';
				server_response[j+6] = 'd';
				server_response[j+7] = '\0';
			} else if ( cp == 2 ) {
				server_response[j] = ' ';
				server_response[j+1] = 'u';
				server_response[j+2] = 'n';
				server_response[j+3] = 'o';
				server_response[j+4] = 'p';
				server_response[j+5] = 'e';
				server_response[j+6] = 'n';
				server_response[j+7] = 'e';
				server_response[j+8] = 'd';
				server_response[j+9] = '\0';
			}
		}

	} else {
		// Unhandled Command
		// data is the command given
		makeUnhandledCommandServerResponse(server_response, message);
		return server_response;
	}

	server_response[0] = code[0];
	server_response[1] = code[1];
	server_response[2] = ';';
	if ( code[0] == 'U' && code[1] == 'E' ) {
		// Unhandled error does not carry any data
		server_response[3] = '\0';
	}
  return server_response;
}

void* threadFunction(void* args){
  thread_arg* arg = args;
  // Buffer for dat  a.
  char message[MAX_MESSAGE_SIZE];

  // Read from the socket and print the contents.
  int nread = read(arg->sock_id,message,MAX_MESSAGE_SIZE);
  while ( nread > 0){
    printf("Received request from client at %s:%d: %s\n",arg->ip, arg->port, message);

  	// @TODO : depending on what the message is, do something.
	// LOCK
    pthread_mutex_lock(node_lock);
  	char* server_response = handleMessage(message);
	// UNLOCK
    pthread_mutex_unlock(node_lock);
    printf("Sending response to client at %s:%d: %s\n", arg->ip, arg->port, server_response);

  	// @TODO : write the right reponse back to client
  	// write back to client, right now this just resends the same exact mesage
  	write(arg->sock_id, server_response, MAX_MESSAGE_SIZE);
    nread = read(arg->sock_id,message,MAX_MESSAGE_SIZE);
  }
  // sleep(5);
  close(arg->sock_id);
  printf("Closed connection with client at %s:%d\n", arg->ip, arg->port );

  return NULL;
}


void accept_connection(int sock, struct sockaddr_in* address, socklen_t* size, thread_arg* t_arg){
  t_arg->sock_id = accept(sock, (struct sockaddr*)address, size);
  if (t_arg->sock_id < 0)
  {
    perror("accept failed");
    //return 1;
  }

  // save client ip and port number
  struct sockaddr_in *client_address = (struct sockaddr_in *) address;
  t_arg->ip = inet_ntoa(client_address->sin_addr);
  t_arg->port = htons(client_address->sin_port);

  printf("Connection initiated from client at %s:%d\n", t_arg->ip, t_arg->port );
  printf("total number of sockets: %d\n",client_num+1);
}


int main(int argc, char** argv) {

  if (argc > NUM_ARGS + 1) {
    printf("Wrong number of args, expected %d, given %d\n", NUM_ARGS, argc - 1);
    exit(1);
  }

  char* dag_file_name = argv[1];
  int port = atoi(argv[2]);

  // process DAG file
  // node_t * root_node = (node_t *) malloc(sizeof(node_t));
  root_node = (node_t *) malloc(sizeof(node_t));
  node_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(node_lock, NULL);
  int num = parseInput(dag_file_name, root_node);
  //printgraph(root_node);


  // Create a TCP socket, bind it to a local address, and listen
  int sock = create_server_socket(port);

  // Declare variables for socket array
  struct sockaddr_in clientAddress[5];
  socklen_t size[5];
  for (int i = 0; i < 5; i++) size[i] = sizeof(struct sockaddr_in);

  // Declare threads
  pthread_t threads[5];
  thread_arg* t_args = (thread_arg*) malloc(sizeof(thread_arg)*5);


  while (1) {
    // FAQ stated "We will not be testing with more than 5 clients.", so this check for client_num is archieved
    //if (client_num >= 5)


    accept_connection(sock, &clientAddress[client_num], &size[client_num], &t_args[client_num]);

		pthread_create(&threads[client_num], NULL, threadFunction, (void*) &t_args[client_num]);

    client_num ++;
  }

  for (int i = 0; i < client_num-1; i++) { pthread_join(threads[i], NULL);}

  // Close the server socket.
  close(sock);
}
