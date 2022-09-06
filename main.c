#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

// Since we mmap the file, we need to offset addresses
#define MEMORY_OFFSET 0x200
#define INSTRUCTIONS_PER_FRAME 9

#define DISPLAY_COLS 64
#define DISPLAY_ROWS 32

// The Chip-8 language is capable of accessing up to 4KB (4,096 bytes) of RAM, from location 0x000 (0) to 0xFFF (4095). The first 512 bytes, from 0x000 to 0x1FF, are where the original interpreter was located, and should not be used by programs.
// Most Chip-8 programs start at location 0x200 (512), but some begin at 0x600 (1536). Programs beginning at 0x600 are intended for the ETI 660 computer.
//
struct chip8 {
  uint8_t v[16];
  uint16_t i;
  uint8_t st;
  uint8_t dt;
  uint16_t pc;
  uint16_t stack[16];
  uint8_t display[DISPLAY_COLS * DISPLAY_ROWS];
  uint8_t *memory;
};

uint8_t *open_file(char *, size_t *);
void draw(struct chip8 *chip8);

int main(int argc, char **argv) {
  if(argc < 2){
    printf("specify a program to run\n");
    exit(0);
  }
  char *file = argv[1];

  size_t len = 0;

  struct chip8 chip8 = {0};
  chip8.pc = MEMORY_OFFSET;
  chip8.memory = open_file(file, &len);

  while (1) {
    for (size_t i = 0; i < INSTRUCTIONS_PER_FRAME*2; i+=2) {
      uint8_t b1 = chip8.memory[i];
      uint8_t b2 = chip8.memory[i+1];
      uint8_t b1lo = b1 & 0xf;
      uint8_t b1hi = b1 >> 4 & 0xf;
      uint8_t b2lo = b2 & 0xf;
      uint8_t b2hi = b2 >> 4 & 0xf;
      printf("%hhx%hhx%hhx%hhx\n", b1hi, b1lo, b2hi, b2lo);
      switch (b1hi) {
        case 0x0:
          if (b1 == 0x00 && b2 == 0xe0) {
            printf("CLS\n");
            break;
          }
          if (b1 == 0x00 && b2 == 0xee) {
            printf("RET\n");
            break;
          }
          printf("SYS %hhx%hhx%hhx\n", b1lo, b2hi, b2lo); // jump to machine language
          break;
        case 0x1:
          printf("JP %hhx%hhx%hhx\n", b1lo, b2hi, b2lo); // jump to address
          break;
        case 0x2:
          printf("CALL %hhx%hhx%hhx\n", b1lo, b2hi, b2lo); // execute subroutine
          break;
        case 0x3:
          printf("SE V%hhx %hhx%hhx\n", b1lo, b2hi, b2lo); // skip if equal
          break;
        case 0x4:
          printf("SNE V%hhx %hhx%hhx\n", b1lo, b2hi, b2lo); // skip if not equal
          break;
        case 0x5:
          printf("SE V%hhx V%hhx %hhx\n", b1lo, b2hi, b2lo); // skip if equal
          break;
        case 0x6:
          printf("LD V%hhx %hhx%hhx\n", b1lo, b2hi, b2lo); // load in register
          break;
        case 0x7:
          printf("ADD V%hhx %hhx%hhx\n", b1lo, b2hi, b2lo); // add constant
          break;
        case 0x8:
          switch (b2lo) {
            case 0x0:
              printf("LD V%hhx V%hhx\n", b1lo, b2hi);
              break;
            case 0x1:
              printf("OR V%hhx V%hhx\n", b1lo, b2hi);
              break;
            case 0x2:
              printf("AND V%hhx V%hhx\n", b1lo, b2hi);
              break;
            case 0x3:
              printf("XOR V%hhx V%hhx\n", b1lo, b2hi);
              break;
            case 0x4:
              printf("ADD V%hhx V%hhx\n", b1lo, b2hi);
              break;
            case 0x5:
              printf("SUB V%hhx V%hhx\n", b1lo, b2hi);
              break;
            case 0x6:
              printf("SHR V%hhx V%hhx\n", b1lo, b2hi);
              break;
            case 0x7:
              printf("SUBN V%hhx V%hhx\n", b1lo, b2hi);
              break;
            case 0xe:
              printf("SHL V%hhx V%hhx\n", b1lo, b2hi);
              break;
            default:
              assert(0);
          }
          break;
        case 0x9:
          switch (b2lo) {
            case 0x0:
              printf("SNE V%hhx V%hhx\n", b1lo, b2hi);
              break;
            default:
              assert(0);
          }
          break;
        case 0xa:
          printf("LD I, %hhx%hhx%hhx\n", b1lo, b2hi, b2lo); // load NNN in register I
          break;
        case 0xb:
          printf("JP V0, %hhx%hhx%hhx\n", b1lo, b2hi, b2lo); // jump to V0 + NNN
          break;
        case 0xc:
          printf("RND V%hhx, %hhx%hhx\n", b1lo, b2hi, b2lo); // Set VX to a random number with a mask of NN
          break;
        case 0xd:
          // Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I
          // Set VF to 01 if any set pixels are changed to unset, and 00 otherwise
          printf("DRW V%hhx V%hhx %hhx\n", b1lo, b2hi, b2lo);
          break;
        case 0xe:
          switch (b2) {
            case 0x9e:
              // Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed
              printf("SKP V%hhx\n", b1lo);
              break;
            case 0xa1:
              // Skip the following instruction if the key corresponding to the hex value currently stored in register VX is not pressed
              printf("SKNP V%hhx\n", b1lo);
              break;
            default:
              printf("%d (of %d)\n", i, len);
              assert(0);
          }
          break;
        case 0xf:
          switch (b2) {
            case 0x07:
              // Store the current value of the delay timer in register VX 
              printf("LD V%hhx, DT\n", b1lo);
              break;
            case 0x0a:
              // Wait for a keypress and store the result in register VX 
              printf("LD V%hhx, K\n", b1lo);
              break;
            case 0x15:
              // Set the delay timer to the value of register VX 
              printf("LD DT, V%hhx\n", b1lo);
              break;
            case 0x18:
              // Set the sound timer to the value of register VX 
              printf("LD ST, V%hhx\n", b1lo);
              break;
            case 0x1e:
              // Add the value stored in register VX to register I 
              printf("ADD I, V%hhx\n", b1lo);
              break;
            case 0x29:
              // Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX 
              printf("LD F, V%hhx\n", b1lo);
              break;
            case 0x33:
              // Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I + 1, and I + 2 
              printf("LD B, V%hhx\n", b1lo);
              break;
            case 0x55:
              // Store the values of registers V0 to VX inclusive in memory starting at address I
              // I is set to I + X + 1 after operation
              printf("LD [I], V%hhx\n", b1lo);
              break;
            case 0x65:
              // Fill registers V0 to VX inclusive with the values stored in memory starting at address I
              // I is set to I + X + 1 after operation
              printf("LD V%hhx, [I]\n", b1lo);
              break;
            default:
              assert(0);
          }
          break;
        default:
          assert(0);
      }
    }

    puts("\e[?1049h");
    draw(&chip8);

    sleep(1.0f/60);
    sleep(5);
    break;
  }

  puts("\e[?1049l");
  return 0;
}

void draw(struct chip8 *chip8) {
  puts("\e[2J");
  for (int r = 0; r < DISPLAY_ROWS; r++) {
    for (int c = 0; c < DISPLAY_COLS; c++) {
      uint8_t pixel = chip8->display[r * DISPLAY_COLS + c];
      printf("%d", pixel ? 1 : 0);
    }
    printf("\n");
  }
}


uint8_t *open_file(char *file, size_t *len) {
  int fd = open(file, O_RDONLY);
  if (fd < 0) {
    printf("failed to open %s\n", file);
    exit(1);
  }

  struct stat statbuf;
  int err = fstat(fd, &statbuf);
  if (err < 0) {
    printf("failed to fstat %s\n", file);
    exit(2);
  }
  *len = statbuf.st_size;

  // Map the file in memory. Add the address space for program execution at the
  // end. We need less than the typical page size (4096, aka 0xfff + 1, minus
  // 0x200). However mmap rounds to page size so that doesn't matter, so we
  // just ask for the whole page.
  uint8_t *ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (ptr == MAP_FAILED) {
    printf("failed to mmap %s\n", file);
    exit(3);
  }
  close(fd);

  // never munmap because we don't care

  return ptr;
}
