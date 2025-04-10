CC=gcc
CFLAGS=-Wall -Wextra

all: compilador assembler executor

compilador: main.c
	$(CC) $(CFLAGS) -o compilador main.c

assembler: assembler.c
	$(CC) $(CFLAGS) -o assembler assembler.c

executor: executor.c
	$(CC) $(CFLAGS) -o executor executor.c

clean:
	rm -f compilador assembler executor
