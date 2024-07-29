CFLAGS=-std=c99 -pedantic -Wall -Wextra -ftrapv -fsanitize=address -fsanitize=undefined -g `sdl2-config --cflags --libs`

.PHONY: run debug runsdl clean

run: terminal
	./terminal chip8-test-suite.ch8 2>/dev/null

debug: terminal
	./terminal chip8-test-suite.ch8 1>/dev/null

terminal: terminal.c

sdl: sdl.c

runsdl: sdl
	./sdl chip8-test-suite.ch8 2>/dev/null

clean:
	rm -rf terminal terminal.dSYM sdl sdl.dSYM
