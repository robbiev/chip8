This is my [CHIP-8](https://en.wikipedia.org/wiki/CHIP-8) emulator.

It has two frontends: a terminal frontend and a SDL2 frontend. The former was
the original frontend. The SDL2 frontend works better.

To run the SDL version, you first need to install SDL2. On macOS, using `brew`:

```
brew install sdl2
```

Then you can run `make runsdl`. Use `make run` to run the terminal version.

The original keyboard layout of the CHIP-8 is as follows:

1 |2 |3 |C
4 |5 |6 |D
7 |8 |9 |E
A |0 |B |F

These are mapped to the keyboard as follows:

1 |2 |3 |4
Q |W |E |R
A |S |D |F
Z |X |C |V
