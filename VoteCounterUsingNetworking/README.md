## Authors

Huangying Kuang, Mitchell Luhm

## The purpose of our program

Allow clients to send commands to a server to manipulate an election vote
counting platform.

## How to compile our program

Change working directory to directory which has `Makefile` `server.c`
`client.c` `client_util.h` `open_read.o` and `makeargv.h`.

Then type `make`.

## How to use the program from the shell

After running `make`, there should be a `server` and `client` executable.

The server program expects two additional command line arguments.

	<arg1> : path to some text file representing the DAG of the entire vote
	         region.
	<arg2> : unsigned integer to be used as port number.

The client program expects three additional command line arguments.

	<arg1> : path to some text file representing all commands to send to the
	         server.
	<arg2> : ip of server to connect to.
	<arg3> : port of server to connect to.

Example:

`./server TestCases/TestCase03/voting_regions.dag 1234`

`./client TestCases/TestCase03/input.req 127.0.0.1 1234`

Note: Server must be running before clients can connect

## What exactly our program does

There is a server that maintains vote counts for all candidates among
several region nodes. It accepts commands client(s) who can request to add
votes to certain regions, remove votes, open polls, close polls, count votes,
and view the winner of the entire overall area.
