## Authors
Mitchell Luhm, Huangying Kuang

## Purpose:

The program tallies base votes in leaf regions by using different programs at
differect sections of a DAG that represent some overall voting region. The
goal of the program is to determine the winner based off multiple votes.txt
files.

Nodes are represented as directories. Leaf nodes have votes.txt files with
the affective votes for certain candidates for that node. Our program starts
at these Leaf votes.txt files, and builds itself up to the root, where we can
decide who the winner is based on votes counted among all the nodes.

## Usage

`./Leaf_Counter <path>`
path specifies the relative or absolute path to a Leaf directory (directory
that contains a votes.txt file AND no sub-directories).

`./Aggregate_Votes <path>`
path could specify any valid node in the DAG. If it is leaf node, a call to
Leaf_Counter program will be invoked. If it is not a leaf node, we will fork
and wait for child processes to execture their program depending on if they are
leaf (Leaf_Counter) or non-leaf (Aggregate_Vote's parent process execution
steps).

`./Vote_Counter <path>`
path is optional. It can specify the path to the root node, or we can assume
the working directory is the root node if a path is not specified. This invokes
Aggregate_Votes program on the root. The unique part of Vote_Counter program,
is it will look at the output eventually generated for the root node, and append
to that file who the winner candidate is.

## Error Handling

1) Empty votes.txt files
Our program is designed to allow some votes.txt files to be empty (with no
candidates and no votes). Essentially, that region with the empty votes.txt file
will have no contribution to the overall result (there are no votes). However,
if we get to the root's output (Who_Won for an example) and its current vote
count (made by Aggregate_Votes called on root) is empty and has no candidates
nor votes, then we report an error since there can't be a winner.
