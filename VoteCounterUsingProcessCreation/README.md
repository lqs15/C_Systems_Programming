# Vote Counter Application

## Authors
- Mitchell Luhm
- Huangying Kuang

## Purpose of the Program
The purpose of this program is get used to programming using basic file IO, fork(), wait(), and other system calls to control the process executions of a vote counting simulation. The "parent" processes (i.e. "County" is a parent of "Subcounty"), must wait for their specific child processes to finishing executing their vote counting programs since their outputs will be used as input for the corresponding parent process.

## How to Compile
We have included a Makefile. Simply clone this repository, navigate to the repository folder and use `make` command to compile. The resulting executable will be called "votecounter".

## How to use Program from Shell
Make sure that the generated executable, `votecounter` and all files unzipped, are in a directory which contains the following: input files for each leaf node, the `input.txt` file for number of candidates and the candidates with their relationships.

Then type `./votecounter "input.txt"` in order to run in regular mode

========== ADDITIONAL COMMAND FOR EXTRA CREDIT  ===========

Type `./votecounter "input.txt" -circle` in order to enable to extra credit feature of detecting cycles in the graph. The program will print error message and exit out

===========================================================

## What the Program Does 
Recursively counts votes from child nodes(lowest region that has raw vote data), sends their outputs as inputs to parents nodes(upper level region that manage a list of regions) until we we reach the root node, we then print the vote counts for all candidates (which also gets put into Output_Who_Won). In the end of this README, we attached a list of functions and global variables of our program.

## Extra Credit Explaination
It traverse the tree and mark each node's status as 1 (visited). If it encounters a node that shouldn't have been visited but already has a status of 1, it marks this node as potential cycle candidate. Then we double check if there is a cycle by checking all descendent of this candidate node, to see if its descendents contain its parent. 
This method distinguishes cases where a child has two different parents from cases where a cycle actually exists.

## Individual Contribution
### Common
parseInput(), parseInputLine(),README

### Mitchell Luhm
- Worked on most of setInitialStatus(), saveCandidates(), execNodes(),detectCycle()
- Allocating, setting, maintaining, deallocating global variables
- Error calls

### Huangying Kuang
saveNodeNames(), getNodeByName(), addChildren(), forkChildren(),doubleCheckCycle()
error handling for input.txt lines


## Global Variables

### int NUMBER_CANDIDATES
- Store the total number of candidates found in the 1st valid line of the `input.txt` file.
- This can be used as a max index to help iterate through ALL_CANDIDATES

### char\*\* ALL_CANDIDATES
- "Array of Strings" for the candidates.
	- i.e. if we have candidates A, B, C, D
		- ALL_CANDIDATES[0] == "A";
		- ALL_CANDIDATES[1] == "B";
		- ... 

### int CURRENT_LINE
- Used as a line index while parsing `input.txt` which ignores the commented lines and empty lines in `input.txt`
	- i.e. if `input.txt`'s first line is a comment, CURRENT_LINE is not incremented after parsing the comment line.
- Needed since we want to process Line 1, 2, 3+ differently

## Functions

### int parseInput(char \*filename, node_t \*n)
- Read each line, send it to parseInputLine

### int parseInputLine(char \*s, node_t \*n, int line_num)
- Receive a line
	- Line 1: call *saveCandidates*
	- Line 2: call *saveNodeNames*
	- Line 3+: call *addChildren*

### void saveNodeNames(char \*s,node_t \*n)
- Create all the nodes
	- Sets their id, name, and output

### node_t\* getNodeByName(char\* name, node_t\* n)
- Return the node with name *name*.

### void addChildren(char \*s,node_t \*n)
- This function called on line 3 and beyond of input.txt
- Divide parent from the children using *makeargv*
- Get the parent once
	- In for loop we go through each child, get the child's id, give it the parent

### void saveCandidates(char \*cand_s) 
- This function called on line 1 of input.txt
- Stores number of candidates in *NUMBER_CANDIDATES*
- Stores a char\* for every candidate as an entry in *ALL_CANDIDATES*. *ALL_CANDIDATES* starts at 0 and has length *NUMBER_CANDIDATES*.

### void setInitialStatus(node_t \*n)
- Go through every node
	- Assign the "prog" value
		- "./find_winner" "./aggregatevotes" "./leafcounter"
	- Assign the "arguments" value
		- Just the arguments for the execv call later, for example input filenames, output filenames, etc

### void execNodes(node_t \*n)
- Calls forkChildren to handle the the majority of the child processes
- Once we are out of forkChildren()
- Fork a final process
	- Parent waits 
	- Final exec done in child process
	- Parent free remaining memory for final cleanup

### void forkChildren(node_t \*n, int n_children)
- This function recursively forks children processes for each node, and execute the correct program for each node, resulting a tree structure for process flow
- The execution starts from the bottom of the tree strucutre, and each parent process waits for all children process to have successfully completed first

### void doubleCheckCycle(node_t \*candidate, node_t \*parent, node_t \*n)
- Recursively check if descendents of candidate has the same id as the id of parent

### void detectCycle(node_t \*n, int i)
- Set n+i's status to 1 (we have visited it)
- Go through the children at node n
	- See if their status has already been set to 1
		- if it has, we have a potential circular dependency so we call doubleCheckCycle to see if it actually is a cycle.

### int main(int argc, char \*\*argv)
- Allocats space for the main node structures
- Checks to make sure there is 2 command line arguments
- Starts the parseInput
- Once input is parsed and nodes parameters are assigned, we start execNodes()
