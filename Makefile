CFLAGS := -std=c99 -g -Wall
CC := gcc
BIN := $(filter-out sh.c, $(wildcard *.c))
BINFILES := $(BIN:.c=.o)

all: sh

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

sh: $(BINFILES) sh.c
	$(CC) $(CFLAGS) $^ -o $@
	objdump -S $@ > $@.asm

run: all
	./sh

clean:
	rm -f sh *.o *.asm

.PHONY: clean run
