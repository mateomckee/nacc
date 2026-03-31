CC = gcc
CFLAGS = -Wall -Wextra -g

SRC = src/main.c src/lexer.c
OUT = nacc

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)
