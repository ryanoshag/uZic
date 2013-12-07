#################################################################
##
## FILE:        Makefile
## PROJECT:        CS 3251 Project 2 - Professor Traynor
## DESCRIPTION: Compile Project 2
##
#################################################################

CC=gcc

OS := $(shell uname -s)

# Extra LDFLAGS if Solaris
ifeq ($(OS), SunOS)
	LDFLAGS=-lsocket -lnsl
endif

all: client server 

client: client.c
	$(CC) musicEncoding.c client.c fileUtil.c -o uZic -g -lpthread -lcrypto

server: server.c
	$(CC) musicEncoding.c server.c fileUtil.c -o uZicServer -g -lpthread -lcrypto

clean:
	rm -f client server *.o