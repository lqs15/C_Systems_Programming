OBJS = open_read.o votecounter.o

all: votecounter

votecounter: $(OBJS)
	gcc $(OBJS) -o votecounter

voutecounter.o: votecounter.c
	gcc -c -o votecounter.o votecounter.c 

clean:
	rm votecounter.o votecounter
