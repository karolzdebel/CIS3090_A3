CC = gcc
CFLAGS = -pthread -g -Wall -pedantic -std=c11

rbs: rbs.c
clean:
	-rm -rf rbs
