CFLAGS := -std=c99 -g -Wall
CC := gcc
BIN := $(filter-out ls.c, $(wildcard *.c))
BINFILES := $(BIN:.c=.o)

all: ls

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

ls: $(BINFILES) ls.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f ls *.o

.PHONY: clean
