#include <stdlib.h> // rand
#define CHIP8_RAND rand
#include "chip8.c"

#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

#include <time.h>
#include <errno.h>

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
      size_t display_pos = r * DISPLAY_COLS + c;
      size_t display_index = display_pos / 8;
      size_t display_bit = display_pos % 8;
      uint8_t display_byte = chip8->display[display_index];
      uint8_t pixel = (display_byte >> (7 - display_bit)) & 1;

      printf("%s", pixel ? "\u2588\u2588" : "  ");
    }
    printf("\n");
  }
}

void print_instruction(struct instruction *instr) {
  // last 12 bit
  uint16_t nnn = ((instr->value[0] & 0xf) << 8) | instr->value[1];

  // last byte
  uint8_t nn = instr->value[1];

  // 4 bit nibbles, excluding the first nibble because it never contains operands
  uint8_t x = instr->value[0] & 0xf;
  uint8_t y = instr->value[1] >> 4 & 0xf;
  uint8_t n = instr->value[1] & 0xf;

  switch (instr->operation) {
    case OP_00E0:
      fprintf(stderr, "CLS\n");
      break;
    case OP_00EE:
      fprintf(stderr, "RET\n");
      break;
    case OP_0NNN:
      fprintf(stderr, "SYS\n");
      break;
    case OP_1NNN:
      fprintf(stderr, "JP %03x\n", nnn); // jump to address
      break;
    case OP_2NNN:
      fprintf(stderr, "CALL %03x\n", nnn); // execute subroutine
      break;
    case OP_3XNN:
      fprintf(stderr, "SEV V%01x %02x\n", x, nn); // skip if equal
      break;
    case OP_4XNN:
      fprintf(stderr, "SNE V%01x %02x\n", x, nn); // skip if not equal
      break;
    case OP_5XY0:
      fprintf(stderr, "SE V%01x V%01x\n", x, y); // skip if equal
      break;
    case OP_6XNN:
      fprintf(stderr, "LD V%01x %02x\n", x, nn); // load in register
      break;
    case OP_7XNN:
      fprintf(stderr, "ADD V%01x %02x\n", x, nn); // add constant
      break;
    case OP_8XY0:
      fprintf(stderr, "LD V%01x V%01x\n", x, y);
      break;
    case OP_8XY1:
      fprintf(stderr, "OR V%01x V%01x\n", x, y);
      break;
    case OP_8XY2:
      fprintf(stderr, "AND V%01x V%01x\n", x, y);
      break;
    case OP_8XY3:
      fprintf(stderr, "XOR V%01x V%01x\n", x, y);
      break;
    case OP_8XY4:
      fprintf(stderr, "ADD V%01x V%01x\n", x, y);
      break;
    case OP_8XY5:
      fprintf(stderr, "SUB V%01x V%01x\n", x, y);
      break;
    case OP_8XY6:
      fprintf(stderr, "SHR V%01x V%01x\n", x, y);
      break;
    case OP_8XY7:
      fprintf(stderr, "SUBN V%01x V%01x\n", x, y);
      break;
    case OP_8XYE:
      fprintf(stderr, "SHL V%01x V%01x\n", x, y);
      break;
    case OP_9XY0:
      fprintf(stderr, "SNE V%01x V%01x\n", x, y);
      break;
    case OP_ANNN:
      fprintf(stderr, "LD I, %03x\n", nnn); // load NNN in register I
      break;
    case OP_BNNN:
      fprintf(stderr, "JP V0, %03x\n", nnn); // jump to V0 + NNN
      break;
    case OP_CXNN:
      fprintf(stderr, "RND V%01x, %02x\n", x, nn); // Set VX to a random number with a mask of NN
      break;
    case OP_DXYN:
      fprintf(stderr, "DRW V%01x V%01x %01x\n", x, y, n);
      break;
    case OP_EX9E:
      fprintf(stderr, "SKP V%01x\n", x);
      break;
    case OP_EXA1:
      fprintf(stderr, "SKNP V%01x\n", x);
      break;
    case OP_FX07:
      fprintf(stderr, "LD V%01x, DT\n", x);
      break;
    case OP_FX0A:
      fprintf(stderr, "LD V%01x, K\n", x);
      break;
    case OP_FX15:
      fprintf(stderr, "LD DT, V%01x\n", x);
      break;
    case OP_FX18:
      fprintf(stderr, "LD ST, V%01x\n", x);
      break;
    case OP_FX1E:
      fprintf(stderr, "ADD I, V%01x\n", x);
      break;
    case OP_FX29:
      fprintf(stderr, "LD F, V%01x\n", x);
      break;
    case OP_FX33:
      fprintf(stderr, "LD B, V%01x\n", x);
      break;
    case OP_FX55:
      fprintf(stderr, "LD [I], V%01x\n", x);
      break;
    case OP_FX65:
      fprintf(stderr, "LD V%01x, [I]\n", x);
      break;
    default:
      fprintf(stderr, "UNKNOWN %02x%02x\n", instr->value[0], instr->value[1]);
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
  //chip8.memory[0x1FF] = 1; // IBM
  //chip8.memory[0x1FF] = 2; // opcodes
  //chip8.memory[0x1FF] = 3; // flags

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
    struct instruction instr = {0};
    for (int i = 0; i < 9; i++) {
      redraw |= cycle(kcode, &chip8, &instr);
      print_instruction(&instr);
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
