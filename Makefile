CC = gcc
CFLAGS = -Wall -Wextra -g -Wno-unused-parameter -Wno-unused-variable

SRC = src/main.c src/util.c src/lexer.c src/parser.c src/sema.c src/tac.c src/codegen_aarch64.c
OUT = nacc

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)
