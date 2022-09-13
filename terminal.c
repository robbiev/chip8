#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

#include <time.h>
#include <errno.h>
#include "chip8.c"

struct termios orig_termios;

void die(const char *s) {
  perror(s);
  //strerror(errno)
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
    die("tcsetattr");
  }
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
    die("tcgetattr");
  }
  atexit(disableRawMode);
  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON);
  raw.c_cc[VMIN] = 0; // number of bytes before read() can return
  raw.c_cc[VTIME] = 0; // max time to wait for read() to return
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    die("tcsetattr");
  }
}

char read_key() {
  char c = -1;
  ssize_t result = 0;
  while ((result = read(STDIN_FILENO, &c, 1)) == -1 && errno == EINTR);
  if (result == -1) {
    die("read");
  }
  return c;
}

int msleep(long msec) {
  struct timespec ts;
  int res;

  if (msec < 0) {
    errno = EINVAL;
    return -1;
  }

  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;

  do {
    res = nanosleep(&ts, &ts);
  } while (res && errno == EINTR);

  return res;
}

size_t read_file(char *file, uint8_t *buffer, size_t buffer_len) {
  FILE *f = fopen(file, "r");
  if (f == NULL) {
    fprintf(stderr, "failed to open %s\n", file);
    exit(1);
  }
  size_t bytes_read = fread(buffer, sizeof *buffer, buffer_len, f);
  if (!feof(f)) {
    fprintf(stderr, "couldn't read file %s\n", file);
    exit(2);
  }
  if (fclose(f) != 0) {
    fprintf(stderr, "failed to close %s\n", file);
    exit(3);
  }
  return bytes_read;
}

void draw(struct chip8 *chip8) {
  puts("\x1b[2J");
  for (int r = 0; r < DISPLAY_ROWS; r++) {
    for (int c = 0; c < DISPLAY_COLS; c++) {
      uint8_t pixel = chip8->display[r * DISPLAY_COLS + c];
      printf("%s", pixel ? "\u2588\u2588" : "  ");
    }
    printf("\n");
  }
}

int main(int argc, char **argv) {
  if (argc < 2){
    fprintf(stderr, "specify a program to run\n");
    exit(0);
  }
  char *file = argv[1];

  struct chip8 chip8 = {0};
  init(&chip8);
  // load the ROM
  read_file(file, &chip8.memory[PROGRAM_START_ADDRESS], (sizeof chip8.memory) - PROGRAM_START_ADDRESS);

  enableRawMode();

  puts("\x1b[?1049h");
  uint64_t key_age_counter = 0;
  uint64_t loop_counter = 0;
  char key = -1;
  for (;;) {
    loop_counter++;
    char k = read_key();
    if (k != -1) {
      key = k;
      key_age_counter = 0;
    }
    char kcode = key_code(key);
    key_age_counter++;

    if (chip8.dt > 0) {
      chip8.dt--;
    }
    if (chip8.st > 0) {
      chip8.st--;
    }

    bool redraw = false;
    for (int i = 0; i < 9; i++) {
      redraw |= cycle(kcode, &chip8);
    }

    if (loop_counter % 5 == 0) {
      draw(&chip8);
    }

    // unset the key if it lasted for 13 cycles
    if (key != -1 && key_age_counter % 13 == 0) {
      key = -1;
      key_age_counter = 0;
    }
    msleep(16);
  }
  puts("\x1b[?1049l");
  return 0;
}
