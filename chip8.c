#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define PROGRAM_START_ADDRESS 0x200

#define DISPLAY_COLS 64
#define DISPLAY_ROWS 32

uint8_t font[] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0,
  0x20, 0x60, 0x20, 0x20, 0x70,
  0xF0, 0x10, 0xF0, 0x80, 0xF0,
  0xF0, 0x10, 0xF0, 0x10, 0xF0,
  0x90, 0x90, 0xF0, 0x10, 0x10,
  0xF0, 0x80, 0xF0, 0x10, 0xF0,
  0xF0, 0x80, 0xF0, 0x90, 0xF0,
  0xF0, 0x10, 0x20, 0x40, 0x40,
  0xF0, 0x90, 0xF0, 0x90, 0xF0,
  0xF0, 0x90, 0xF0, 0x10, 0xF0,
  0xF0, 0x90, 0xF0, 0x90, 0x90,
  0xE0, 0x90, 0xE0, 0x90, 0xE0,
  0xF0, 0x80, 0x80, 0x80, 0xF0,
  0xE0, 0x90, 0x90, 0x90, 0xE0,
  0xF0, 0x80, 0xF0, 0x80, 0xF0,
  0xF0, 0x80, 0xF0, 0x80, 0x80,
};

struct chip8 {
  uint8_t v[16];
  uint16_t i;
  uint8_t st;
  uint8_t dt;
  uint16_t pc;
  uint8_t sp;
  uint16_t stack[16];
  uint8_t display[DISPLAY_COLS * DISPLAY_ROWS];
  uint8_t memory[4096];
};

void init(struct chip8 *chip8) {
  memcpy(chip8->memory, font, sizeof(font));
  chip8->pc = PROGRAM_START_ADDRESS;
  //chip8.pc = 0x250;

  //chip8.memory[0x1FF] = 1; // IBM
  //chip8.memory[0x1FF] = 2; // opcodes
  //chip8.memory[0x1FF] = 3; // flags
}

bool cycle(char key, struct chip8 *chip8) {
  bool redraw = false;
  fprintf(stderr, "key: %d, V0: %d, V1: %d\n", key, chip8->v[0], chip8->v[1]);
  uint8_t b1 = chip8->memory[chip8->pc];
  uint8_t b2 = chip8->memory[chip8->pc + 1];
  uint16_t new_pc = chip8->pc + 2;

  uint8_t b1lo = b1 & 0xf;
  uint8_t b1hi = b1 >> 4 & 0xf;
  uint8_t b2lo = b2 & 0xf;
  uint8_t b2hi = b2 >> 4 & 0xf;
  //fprintf(stderr, "%hhx%hhx%hhx%hhx\n", b1hi, b1lo, b2hi, b2lo);
  switch (b1hi) {
    case 0x0:
      if (b1 == 0x00 && b2 == 0xe0) {
        fprintf(stderr, "CLS\n");
        memset(chip8->display, 0, sizeof(chip8->display));
        redraw = true;
        break;
      }
      if (b1 == 0x00 && b2 == 0xee) {
        fprintf(stderr, "RET\n");
        new_pc = chip8->stack[chip8->sp];
        new_pc += 2;
        chip8->sp--;
        break;
      }
      fprintf(stderr, "SYS %hhx%hhx%hhx\n", b1lo, b2hi, b2lo); // jump to machine language
      // ignored by modern impls
      break;
    case 0x1:
      fprintf(stderr, "JP %hhx%hhx%hhx\n", b1lo, b2hi, b2lo); // jump to address
      new_pc = (b1lo << 8) | b2;
      break;
    case 0x2:
      fprintf(stderr, "CALL %hhx%hhx%hhx\n", b1lo, b2hi, b2lo); // execute subroutine
      chip8->sp++;
      chip8->stack[chip8->sp] = chip8->pc;
      new_pc = (b1lo << 8) | b2;
      break;
    case 0x3:
      fprintf(stderr, "SE V%hhx %hhx%hhx\n", b1lo, b2hi, b2lo); // skip if equal
      if (chip8->v[b1lo] == b2) {
        new_pc += 2;
      }
      break;
    case 0x4:
      fprintf(stderr, "SNE V%hhx %hhx%hhx\n", b1lo, b2hi, b2lo); // skip if not equal
      if (chip8->v[b1lo] != b2) {
        new_pc += 2;
      }
      break;
    case 0x5:
      fprintf(stderr, "SE V%hhx V%hhx %hhx\n", b1lo, b2hi, b2lo); // skip if equal
      if (chip8->v[b1lo] == chip8->v[b2hi]) {
        new_pc += 2;
      }
      break;
    case 0x6:
      fprintf(stderr, "LD V%hhx %hhx%hhx\n", b1lo, b2hi, b2lo); // load in register
      chip8->v[b1lo] = b2;
      break;
    case 0x7:
      fprintf(stderr, "ADD V%hhx %hhx%hhx\n", b1lo, b2hi, b2lo); // add constant
      chip8->v[b1lo] += b2;
      break;
    case 0x8:
      switch (b2lo) {
        case 0x0:
          fprintf(stderr, "LD V%hhx V%hhx\n", b1lo, b2hi);
          chip8->v[b1lo] = chip8->v[b2hi];
          break;
        case 0x1:
          fprintf(stderr, "OR V%hhx V%hhx\n", b1lo, b2hi);
          chip8->v[b1lo] |= chip8->v[b2hi];
          break;
        case 0x2:
          fprintf(stderr, "AND V%hhx V%hhx\n", b1lo, b2hi);
          chip8->v[b1lo] &= chip8->v[b2hi];
          break;
        case 0x3:
          fprintf(stderr, "XOR V%hhx V%hhx\n", b1lo, b2hi);
          chip8->v[b1lo] ^= chip8->v[b2hi];
          break;
        case 0x4:
          fprintf(stderr, "ADD V%hhx V%hhx\n", b1lo, b2hi);
          {
            uint8_t vx = chip8->v[b1lo];
            uint8_t vy = chip8->v[b2hi];
            if ((uint8_t)(vx + vy) < vx) {
              chip8->v[0xf] = 1;
            } else {
              chip8->v[0xf] = 0;
            }
            chip8->v[b1lo] = vx + vy;
            break;
          }
        case 0x5:
          fprintf(stderr, "SUB V%hhx V%hhx\n", b1lo, b2hi);
          {
            uint8_t vx = chip8->v[b1lo];
            uint8_t vy = chip8->v[b2hi];
            if (vy > vx)  {
              chip8->v[0xf] = 0;
            } else {
              chip8->v[0xf] = 1;
            }
            chip8->v[b1lo] = vx - vy;
          }
          break;
        case 0x6:
          fprintf(stderr, "SHR V%hhx V%hhx\n", b1lo, b2hi);
          {
            uint8_t vy = chip8->v[b2hi];
            chip8->v[b1lo] = vy >> 1;
            chip8->v[0xf] = vy & 0x01;
          }
          break;
        case 0x7:
          fprintf(stderr, "SUBN V%hhx V%hhx\n", b1lo, b2hi);
          {
            uint8_t vx = chip8->v[b1lo];
            uint8_t vy = chip8->v[b2hi];
            if (vx > vy)  {
              chip8->v[0xf] = 0;
            } else {
              chip8->v[0xf] = 1;
            }
            chip8->v[b1lo] = vy - vx;
          }
          break;
        case 0xe:
          fprintf(stderr, "SHL V%hhx V%hhx\n", b1lo, b2hi);
          {
            uint8_t vy = chip8->v[b2hi];
            chip8->v[b1lo] = vy << 1;
            chip8->v[0xf] = (vy & 0x80) != 0;
          }
          break;
        default:
          assert(0);
      }
      break;
    case 0x9:
      switch (b2lo) {
        case 0x0:
          fprintf(stderr, "SNE V%hhx V%hhx\n", b1lo, b2hi);
          if (chip8->v[b1lo] != chip8->v[b2hi]) {
            new_pc += 2;
          }
          break;
        default:
          assert(0);
      }
      break;
    case 0xa:
      fprintf(stderr, "LD I, %hhx%hhx%hhx\n", b1lo, b2hi, b2lo); // load NNN in register I
      chip8->i = (b1lo << 8) | b2;
      break;
    case 0xb:
      fprintf(stderr, "JP V0, %hhx%hhx%hhx\n", b1lo, b2hi, b2lo); // jump to V0 + NNN
      new_pc = ((b1lo << 8) | b2) + chip8->v[0];
      break;
    case 0xc:
      fprintf(stderr, "RND V%hhx, %hhx%hhx\n", b1lo, b2hi, b2lo); // Set VX to a random number with a mask of NN
      chip8->v[b1lo] = rand() & b2;
      break;
    case 0xd:
      // Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I
      // Set VF to 01 if any set pixels are changed to unset, and 00 otherwise
      fprintf(stderr, "DRW V%hhx V%hhx %hhx\n", b1lo, b2hi, b2lo);
      {
        redraw = true;
        uint8_t col = chip8->v[b1lo] & (DISPLAY_COLS - 1);
        uint8_t row = chip8->v[b2hi] & (DISPLAY_ROWS - 1);
        //fprintf(stderr, "row: %d, col: %d, count: %d, address: %d\n", row, col, b2lo, chip8->i);
        uint16_t address = chip8->i;
        chip8->v[0xf] = 0;
        for (int i = 0; i < b2lo && row < DISPLAY_ROWS; i++) {
          uint8_t sprite_data = chip8->memory[address];

          for (int bit = 7; bit >= 0; bit--) {
            if (col + (7-bit) >= DISPLAY_COLS) {
              continue;
            }
            uint8_t value = (sprite_data >> bit) & 1;
            uint8_t prev = chip8->display[(row * DISPLAY_COLS) + col + (7-bit)];
            uint8_t new = prev ^ value;
            chip8->display[(row * DISPLAY_COLS) + col + (7-bit)] = new;
            if (prev == 1 && new == 0) {
              chip8->v[0xf] = 1;
            }
          }
          address++;
          row++;
        }
      }
      break;
    case 0xe:
      switch (b2) {
        case 0x9e:
          // Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed
          fprintf(stderr, "SKP V%hhx\n", b1lo);
          if (chip8->v[b1lo] == key) {
            fprintf(stderr, "SKP? %d %d\n", chip8->v[b1lo], key);
            new_pc += 2;
          }
          break;
        case 0xa1:
          // Skip the following instruction if the key corresponding to the hex value currently stored in register VX is not pressed
        {
          fprintf(stderr, "SKNP V%hhx\n", b1lo);
          //fprintf(stderr, "XXX SKNP %hhx\n", chip8->v[b1lo]);
          if (chip8->v[b1lo] != key) {
            //fprintf(stderr, "SKNP? %d %d\n", chip8->v[b1lo], key);
            new_pc += 2;
          }
        }
          break;
        default:
          assert(0);
      }
      break;
    case 0xf:
      switch (b2) {
        case 0x07:
          // Store the current value of the delay timer in register VX
          fprintf(stderr, "LD V%hhx, DT\n", b1lo);
          chip8->v[b1lo] = chip8->dt;
          break;
        case 0x0a:
          // Wait for a keypress and store the result in register VX
          fprintf(stderr, "LD V%hhx, K\n", b1lo);
          if (key == -1) {
            new_pc = chip8->pc;
          } else {
            chip8->v[b1lo] = key;
          }
          break;
        case 0x15:
          // Set the delay timer to the value of register VX
          fprintf(stderr, "LD DT, V%hhx\n", b1lo);
          chip8->dt = chip8->v[b1lo];
          break;
        case 0x18:
          // Set the sound timer to the value of register VX
          fprintf(stderr, "LD ST, V%hhx\n", b1lo);
          chip8->st = chip8->v[b1lo];
          break;
        case 0x1e:
          // Add the value stored in register VX to register I
          fprintf(stderr, "ADD I, V%hhx\n", b1lo);
          chip8->i += chip8->v[b1lo];
          break;
        case 0x29:
          // Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX
          // TODO font data
          fprintf(stderr, "LD F, V%hhx\n", b1lo);
          chip8->i = b1lo * 5;
          break;
        case 0x33:
          // Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I + 1, and I + 2
          fprintf(stderr, "LD B, V%hhx\n", b1lo);
          //assert(0);
          break;
        case 0x55:
          // Store the values of registers V0 to VX inclusive in memory starting at address I
          // I is set to I + X + 1 after operation
          fprintf(stderr, "LD [I], V%hhx\n", b1lo);
          for (int i = 0; i <= b1lo; i++) {
            chip8->memory[chip8->i + i] = chip8->v[i];
          }
          chip8->i = chip8->i + b1lo + 1;
          break;
        case 0x65:
          // Fill registers V0 to VX inclusive with the values stored in memory starting at address I
          // I is set to I + X + 1 after operation
          fprintf(stderr, "LD V%hhx, [I]\n", b1lo);
          for (int i = 0; i <= b1lo; i++) {
            chip8->v[i] = chip8->memory[chip8->i + i];
          }
          chip8->i = chip8->i + b1lo + 1;
          break;
        default:
          assert(0);
      }
      break;
    default:
      assert(0);
  }
  chip8->pc = new_pc;
  return redraw;
}

char key_code(char key) {
  switch (key) {
    case '1': return 0x1;
    case '2': return 0x2;
    case '3': return 0x3;
    case 'q': return 0x4;
    case 'w': return 0x5;
    case 'e': return 0x6;
    case 'a': return 0x7;
    case 's': return 0x8;
    case 'd': return 0x9;
    case 'z': return 0xA;
    case 'x': return 0x0;
    case 'c': return 0xB;
    case '4': return 0xC;
    case 'r': return 0xD;
    case 'f': return 0xE;
    case 'v': return 0xF;
  }
  return -1;
}
