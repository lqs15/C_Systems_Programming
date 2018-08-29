all: votecounter

votecounter: votecounter.c
	gcc -std=c99 -pthread open_read.o -o votecounter votecounter.c

clean:
	rm votecounter
