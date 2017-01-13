P=8080

all: server.c
	- gcc -lpthread -g -o server server.c

run: server
	- ./server $P

clean:
	- rm -rf server

.PHONY = all clean

