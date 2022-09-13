CFLAGS=-std=c99 -pedantic -Wall -Wextra -ftrapv -fsanitize=address -fsanitize=undefined -g

.PHONY=run
run: terminal
	./terminal chip8-test-suite.ch8 2>/dev/null

.PHONY=debug
debug: terminal
	./terminal chip8-test-suite.ch8 1>/dev/null

terminal: terminal.c

.PHONY=clean
clean:
	rm -rf terminal terminal.dSYM
