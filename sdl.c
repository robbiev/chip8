#include <stdlib.h> // rand
#define CHIP8_RAND rand
#include "chip8.c"

#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 320

void die(char *s) {
  perror(s);
  exit(1);
}

size_t read_file(char *file, uint8_t *buffer, size_t buffer_len) {
  FILE *f = fopen(file, "r");
  if (f == NULL) {
    die("fopen");
  }
  size_t bytes_read = fread(buffer, sizeof *buffer, buffer_len, f);
  if (!feof(f)) {
    die("fread");
  }
  if (fclose(f) != 0) {
    die("fclose");
  }
  return bytes_read;
}

int main(int argc, char **argv) {
  if (argc < 2){
    fprintf(stderr, "specify a program to run\n");
    exit(1);
  }
  char *file = argv[1];

  struct chip8 chip8 = {0};
  chip8_init(&chip8);
  // load the ROM
  read_file(file, &chip8.memory[PROGRAM_START_ADDRESS], (sizeof chip8.memory) - PROGRAM_START_ADDRESS);

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window * window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_UNDEFINED,
                                         SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

  SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
  int width = SCREEN_WIDTH;
  int height = SCREEN_HEIGHT;

  SDL_SetWindowMinimumSize(window, width, height);
  SDL_RenderSetLogicalSize(renderer, width, height);
  SDL_RenderSetIntegerScale(renderer, 1);

  SDL_Surface * surface = SDL_CreateRGBSurfaceWithFormat(SDL_SWSURFACE,
                                                         DISPLAY_COLS, DISPLAY_ROWS, 1, SDL_PIXELFORMAT_INDEX1MSB);
  SDL_Color colors[2] = {{0, 0, 0, 255}, {255, 255, 255, 255}};
  SDL_SetPaletteColors(surface->format->palette, colors, 0, 2);
  surface->pixels = &chip8.display;

  bool done = false;
  SDL_Event event;
  int f = 60;

  bool redraw = false;

  while (!done) {
    Uint32 start = SDL_GetTicks();

    while (SDL_PollEvent(&event) ) {
      if (event.type == SDL_QUIT) {
        done = true;
      }
      if (event.type == SDL_KEYDOWN) {
        char c = event.key.keysym.sym;
        chip8_key_code_down(&chip8, chip8_key_to_key_code(c));
      }
      if (event.type == SDL_KEYUP) {
        char c = event.key.keysym.sym;
        chip8_key_code_up(&chip8, chip8_key_to_key_code(c));
      }
    }

    chip8_60hz_timer(&chip8);

    struct cycle_result res = {0};
    for (int i = 0; i < 30; i++) {
      cycle(&chip8, &res);
      redraw |= res.redraw_needed;
    }

    if (redraw) {
      SDL_RenderClear(renderer);
      SDL_Texture *screen_texture = SDL_CreateTextureFromSurface(renderer, surface);
      SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
      SDL_RenderPresent(renderer);
      SDL_DestroyTexture(screen_texture);
      redraw = false;
    }

    Uint32 end = SDL_GetTicks();
    int wait_for = 1000 / f - (end - start);
    if (wait_for > 0) {
      SDL_Delay(wait_for);
    }
  }
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
