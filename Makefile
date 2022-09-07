CFLAGS=-std=c99 -pedantic -Wall -Wextra -ftrapv -fsanitize=address -fsanitize=undefined -g

.PHONY=run
run: main
	./main chip8-test-suite.ch8 2>/dev/null

.PHONY=debug
debug: main
	./main chip8-test-suite.ch8 1>/dev/null

main: main.c

.PHONY=clean
clean:
	rm -f main
