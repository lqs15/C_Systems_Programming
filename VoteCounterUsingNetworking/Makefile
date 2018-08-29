
all: server client

server: server.c
	gcc -std=c99 -pthread open_read.o -o server server.c

client: client.c
	gcc -std=c99 -pthread open_read.o -o client client.c

clean:
	rm server client
