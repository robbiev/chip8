CFLAGS=-std=c99 -pedantic -Wall -Wextra -ftrapv -fsanitize=address -fsanitize=undefined -g
.PHONY=run
run: main
	./main chip8-test-suite.ch8
main: main.c
