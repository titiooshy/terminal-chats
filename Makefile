# Compiler and flags
CC = gcc
CFLAGS = -Wall -pthread

# Targets
all: server person

# Build server
server: server.c
	$(CC) $(CFLAGS) server.c -o server

# Build person
person: person.c
	$(CC) $(CFLAGS) person.c -o person

# Clean compiled files
clean:
	rm -f server person
