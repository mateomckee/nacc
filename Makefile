CC = gcc
CFLAGS = -Wall -Wextra -g

SRC = src/main.c src/util.c src/lexer.c src/parser.c src/sema.c
OUT = nacc

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)
