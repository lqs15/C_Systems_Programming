## Authors

Huangying Kuang, Mitchell Luhm

## The purpose of our program

To utilize threads and synchronization techinques to count and aggregate votes
of a collection of nodes in a directed acyclic graph.

## How to compile our program

Change working directory to directory which has `makefile` `votecounter.c`
and `makeargv.h`. Then type `make`.

## How to use the program from the shell

After running `make`, there should be a `votecounter` executable. The
votecounter program expects three additional command line arguments.

	<arg1> : path to some .txt file representing the DAG
	<arg2> : path to an input directory containing files for leaf nodes and
		     their votes.
	<arg3> : path to an output directory where aggregation of votes will be
		     written to.

Example:

`./votecounter TestCase01/input.txt TestCase01/inputdir/ TestCase01/outputdir`

## What exactly our program does

Counts votes given from leaf nodes. One thread is spawned for every leaf node
in the DAG. All ancestors of the leaf will be visited individually by each
threads in aggregate to the aggregation result files throughout the output
directory specified by <arg3>. Three categories of locks were created: lock for log.txt,
lock for each node, lock for shared queue. This allows us to update files, shared
resources safely.
