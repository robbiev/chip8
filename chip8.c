#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))
#define PROGRAM_START_ADDRESS 0x200

#define DISPLAY_COLS 64
#define DISPLAY_ROWS 32
#define DISPLAY_BYTES ((DISPLAY_COLS * DISPLAY_ROWS) / 8)

#ifndef CHIP8_RAND
#define CHIP8_RAND ((void)0))
#endif

#define CHIP8_KEY_CODE_NO_KEY 0x1F
#define CHIP8_KEY_CODE_EVENT_WANTED 0x2F

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
  uint8_t memory[4096];
  uint8_t display[DISPLAY_BYTES];
  uint16_t stack[16];
  uint8_t v[16];

  // bits for pressed keys, from F..0 (msb = key F, lsb = key 0)
  uint16_t keys_currently_pressed;

  uint16_t pc;
  uint16_t i;
  uint8_t sp;
  uint8_t st;
  uint8_t dt;

  // the last key that was released, or CHIP8_KEY_CODE_NO_KEY
  uint8_t last_key_released_event;
};

// Every key can have two events max, press - depress. Repetition overwrites.
// Goes from depress -> press -> depress -> press
// However we want to know if if a depress happened

// has a key been released since the last cycle, and which one
// has a key been released since the first time I ran

enum opcode {
  OP_00E0,
  OP_00EE,
  OP_0NNN,
  OP_1NNN,
  OP_2NNN,
  OP_3XNN,
  OP_4XNN,
  OP_5XY0,
  OP_6XNN,
  OP_7XNN,
  OP_8XY0,
  OP_8XY1,
  OP_8XY2,
  OP_8XY3,
  OP_8XY4,
  OP_8XY5,
  OP_8XY6,
  OP_8XY7,
  OP_8XYE,
  OP_9XY0,
  OP_ANNN,
  OP_BNNN,
  OP_CXNN,
  OP_DXYN,
  OP_EX9E,
  OP_EXA1,
  OP_FX07,
  OP_FX0A,
  OP_FX15,
  OP_FX18,
  OP_FX1E,
  OP_FX29,
  OP_FX33,
  OP_FX55,
  OP_FX65,
};

struct instruction {
  uint8_t value[2];
  enum opcode operation;
};

struct cycle_result {
  struct instruction instr;
  bool redraw_needed;
};

void chip8_key_code_down(struct chip8 *chip8, uint8_t key_code) {
  chip8->keys_currently_pressed |= (1 << key_code);
}

void chip8_key_code_up(struct chip8 *chip8, uint8_t key_code) {
  chip8->keys_currently_pressed &= ~(1 << key_code);
  if (chip8->last_key_released_event == CHIP8_KEY_CODE_EVENT_WANTED) {
    chip8->last_key_released_event = key_code;
  }
}

bool chip8_is_key_code_pressed(struct chip8 *chip8, uint8_t key_code) {
  return (chip8->keys_currently_pressed & (1 << key_code)) != 0;
}

void chip8_init(struct chip8 *chip8) {
  memcpy(chip8->memory, font, sizeof(font));
  chip8->pc = PROGRAM_START_ADDRESS;
  chip8->sp = ARRAY_LEN(chip8->stack);
  chip8->last_key_released_event = CHIP8_KEY_CODE_NO_KEY;
}

size_t min(size_t a, size_t b) {
  if (a < b) {
    return a;
  }
  return b;
}

void cycle(struct chip8 *chip8, struct cycle_result *res) {
  uint8_t b1 = chip8->memory[chip8->pc];
  uint8_t b2 = chip8->memory[chip8->pc + 1];
  uint16_t new_pc = chip8->pc + 2;

  uint8_t b1lo = b1 & 0xf;
  uint8_t b1hi = b1 >> 4 & 0xf;
  uint8_t b2lo = b2 & 0xf;
  uint8_t b2hi = b2 >> 4 & 0xf;

  res->instr.value[0] = b1;
  res->instr.value[1] = b2;

  res->redraw_needed = false;

  switch (b1hi) {
    case 0x0:
      if (b1 == 0x00 && b2 == 0xe0) {
        res->instr.operation = OP_00E0;
        memset(chip8->display, 0, sizeof(chip8->display));
        res->redraw_needed = true;
        break;
      }
      if (b1 == 0x00 && b2 == 0xee) {
        res->instr.operation = OP_00EE;
        new_pc = chip8->stack[chip8->sp];
        new_pc += 2;
        chip8->sp++;
        break;
      }
      res->instr.operation = OP_0NNN; // ignored instruction (jump to machine code)
      break;
    case 0x1:
      res->instr.operation = OP_1NNN;
      new_pc = (b1lo << 8) | b2;
      break;
    case 0x2:
      res->instr.operation = OP_2NNN;
      chip8->sp--;
      chip8->stack[chip8->sp] = chip8->pc;
      new_pc = (b1lo << 8) | b2;
      break;
    case 0x3:
      res->instr.operation = OP_3XNN;
      if (chip8->v[b1lo] == b2) {
        new_pc += 2;
      }
      break;
    case 0x4:
      res->instr.operation = OP_4XNN;
      if (chip8->v[b1lo] != b2) {
        new_pc += 2;
      }
      break;
    case 0x5:
      res->instr.operation = OP_5XY0;
      if (chip8->v[b1lo] == chip8->v[b2hi]) {
        new_pc += 2;
      }
      break;
    case 0x6:
      res->instr.operation = OP_6XNN;
      chip8->v[b1lo] = b2;
      break;
    case 0x7:
      res->instr.operation = OP_7XNN;
      chip8->v[b1lo] += b2;
      break;
    case 0x8:
      switch (b2lo) {
        case 0x0:
          res->instr.operation = OP_8XY0;
          chip8->v[b1lo] = chip8->v[b2hi];
          break;
        case 0x1:
          res->instr.operation = OP_8XY1;
          chip8->v[b1lo] |= chip8->v[b2hi];
          chip8->v[0xf] = 0;
          break;
        case 0x2:
          res->instr.operation = OP_8XY2;
          chip8->v[b1lo] &= chip8->v[b2hi];
          chip8->v[0xf] = 0;
          break;
        case 0x3:
          res->instr.operation = OP_8XY3;
          chip8->v[b1lo] ^= chip8->v[b2hi];
          chip8->v[0xf] = 0;
          break;
        case 0x4: {
          res->instr.operation = OP_8XY4;
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
        case 0x5: {
          res->instr.operation = OP_8XY5;
          uint8_t vx = chip8->v[b1lo];
          uint8_t vy = chip8->v[b2hi];
          if (vy > vx)  {
            chip8->v[0xf] = 0;
          } else {
            chip8->v[0xf] = 1;
          }
          chip8->v[b1lo] = vx - vy;
          break;
        }
        case 0x6: {
          res->instr.operation = OP_8XY6;
            uint8_t vy = chip8->v[b2hi];
            chip8->v[b1lo] = vy >> 1;
            chip8->v[0xf] = vy & 0x01;
          break;
        }
        case 0x7: {
          res->instr.operation = OP_8XY7;
          uint8_t vx = chip8->v[b1lo];
          uint8_t vy = chip8->v[b2hi];
          if (vx > vy)  {
            chip8->v[0xf] = 0;
          } else {
            chip8->v[0xf] = 1;
          }
          chip8->v[b1lo] = vy - vx;
          break;
        }
        case 0xe: {
          res->instr.operation = OP_8XYE;
          uint8_t vy = chip8->v[b2hi];
          chip8->v[b1lo] = vy << 1;
          chip8->v[0xf] = (vy & 0x80) != 0;
          break;
        }
        default:
          assert(0);
      }
      break;
    case 0x9:
      switch (b2lo) {
        case 0x0:
          res->instr.operation = OP_9XY0;
          if (chip8->v[b1lo] != chip8->v[b2hi]) {
            new_pc += 2;
          }
          break;
        default:
          assert(0);
      }
      break;
    case 0xa:
      res->instr.operation = OP_ANNN;
      chip8->i = (b1lo << 8) | b2;
      break;
    case 0xb:
      res->instr.operation = OP_BNNN;
      new_pc = ((b1lo << 8) | b2) + chip8->v[0];
      break;
    case 0xc:
      res->instr.operation = OP_CXNN;
      chip8->v[b1lo] = CHIP8_RAND() & b2;
      break;
    case 0xd: {
      res->instr.operation = OP_DXYN;
      // Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I
      // Set VF to 01 if any set pixels are changed to unset, and 00 otherwise
      res->redraw_needed = true;
      uint8_t col = chip8->v[b1lo] & (DISPLAY_COLS - 1);
      uint8_t row = chip8->v[b2hi] & (DISPLAY_ROWS - 1);
      uint16_t address = chip8->i;
      chip8->v[0xf] = 0;
      for (size_t i = 0; i < b2lo && row < DISPLAY_ROWS; i++) {
        uint8_t sprite_data = chip8->memory[address];

        for (size_t bit = 0; bit < min(DISPLAY_COLS - col, 8); bit++) {
          size_t display_pos = (row * DISPLAY_COLS) + col + bit;
          uint8_t prev_byte = chip8->display[display_pos / 8];
          uint8_t prev_bit = prev_byte >> (7 - (display_pos % 8)) & 1;
          uint8_t sprite_bit = (sprite_data >> (7 - bit)) & 1;
          uint8_t new_bit = prev_bit ^ sprite_bit;
          uint8_t prev_byte_cleared_bit = prev_byte & ~(1 << (7 - (display_pos % 8)));
          uint8_t new_byte = prev_byte_cleared_bit | (new_bit << (7 - (display_pos % 8)));
          chip8->display[display_pos / 8] = new_byte;
          if (prev_bit == 1 && new_bit == 0) {
            chip8->v[0xf] = 1;
          }
        }
        address++;
        row++;
      }
      break;
    }
    case 0xe:
      switch (b2) {
        case 0x9e:
          res->instr.operation = OP_EX9E;
          // Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed
          if (chip8_is_key_code_pressed(chip8, chip8->v[b1lo])) {
            new_pc += 2;
          }
          break;
        case 0xa1:
          res->instr.operation = OP_EXA1;
          // Skip the following instruction if the key corresponding to the hex value currently stored in register VX is not pressed
          if (!chip8_is_key_code_pressed(chip8, chip8->v[b1lo])) {
            new_pc += 2;
          }
          break;
        default:
          assert(0);
      }
      break;
    case 0xf:
      switch (b2) {
        case 0x07:
          res->instr.operation = OP_FX07;
          // Store the current value of the delay timer in register VX
          chip8->v[b1lo] = chip8->dt;
          break;
        case 0x0a:
          res->instr.operation = OP_FX0A;
          // Wait for a keypress and store the result in register VX
          if (chip8->last_key_released_event == CHIP8_KEY_CODE_NO_KEY || chip8->last_key_released_event == CHIP8_KEY_CODE_EVENT_WANTED) {
            chip8->last_key_released_event = CHIP8_KEY_CODE_EVENT_WANTED;
            new_pc = chip8->pc;
          } else {
            chip8->v[b1lo] = chip8->last_key_released_event;
            chip8->last_key_released_event = CHIP8_KEY_CODE_NO_KEY;
          }
          break;
        case 0x15:
          res->instr.operation = OP_FX15;
          // Set the delay timer to the value of register VX
          chip8->dt = chip8->v[b1lo];
          break;
        case 0x18:
          res->instr.operation = OP_FX18;
          // Set the sound timer to the value of register VX
          chip8->st = chip8->v[b1lo];
          break;
        case 0x1e:
          res->instr.operation = OP_FX1E;
          // Add the value stored in register VX to register I
          chip8->i += chip8->v[b1lo];
          break;
        case 0x29:
          res->instr.operation = OP_FX29;
          // Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX
          chip8->i = b1lo * 5;
          break;
        case 0x33:
          res->instr.operation = OP_FX33;
          // Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I + 1, and I + 2
          chip8->memory[chip8->i] = chip8->v[b1lo] / 100;
          chip8->memory[chip8->i + 1] = (chip8->v[b1lo] / 10) % 10;
          chip8->memory[chip8->i + 2] = chip8->v[b1lo] % 10;
          break;
        case 0x55:
          res->instr.operation = OP_FX55;
          // Store the values of registers V0 to VX inclusive in memory starting at address I
          // I is set to I + X + 1 after operation
          for (int i = 0; i <= b1lo; i++) {
            chip8->memory[chip8->i + i] = chip8->v[i];
          }
          chip8->i = chip8->i + b1lo + 1;
          break;
        case 0x65:
          res->instr.operation = OP_FX65;
          // Fill registers V0 to VX inclusive with the values stored in memory starting at address I
          // I is set to I + X + 1 after operation
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
}

uint8_t chip8_key_to_key_code(char key) {
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
    default: return CHIP8_KEY_CODE_NO_KEY;
  }
}
