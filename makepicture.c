/* Makepicture.c
 *
 * Takes a maze and string (argv || stdin) and converts it to a picture of the
 * maze as it was solved.
 *
 * String format: A series of moves. UDLR (Up Down Left Right)
 *   Todo: Add more commands?
 * Each command probably adds a frame to the animated gif output.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>

char *slurp(FILE *fd) {
  int c;
  int len = 64;
  int pos = 0;
  char *s = malloc(len);
  while( (c = fgetc(fd)) != EOF ) {
    if(pos == len) {
      len *= 2;
      s = realloc(s, len);
    }
    s[pos] = c;
    pos++;
  }
  s[pos] = '\0';
  return s;
}

void dumphelp(void) {
  puts("You're doing it wrong");
  exit(1);
}

int main(int argc, char **argv) {
  char *maze, *commands;
  if(2 == argc) {
    commands = argv[1];
    maze = slurp(stdin);
  } else if(3 == argc) {
    commands = argv[1];
    maze = slurp(fopen(argv[2], "r"));
  } else {
    dumphelp();
  }
  assert(maze && commands);
  printf("Commands: '''%s''', Maze: '''%s'''\n", commands, maze);
 /* Start Writing GIF. 6 byte header & version is GIF89a */
  FILE *output = fopen("solved.gif", "w");
  assert(output);
  assert(6 == fwrite("GIF89a", 1, 6, output));

  uint16_t dimensions[2] = {10, 10};
 /* Width and height are 16 bit uints, little endian */
  fwrite(dimensions, 2, 2, output);
  /*  0b10010001 flags with image settings
   *    |\|/|\|/
   *    | | | \-- (3 bits) Size of global Color Table
   *    | | \---- (1 bit)  Sort flag (something about colour sorting?!)
   *    | \------ (3 bits) Colour resolution: 2^(n+1), so here 4.
   *    \-------- (1 bit)  Global Colour table flag; whether there is a colour table
   */
  char flags = 0x91;
  assert(1 == fwrite(&flags, 1, 1, output));
  char bgcolour = 0;
  char pixelratio = 0;
  fwrite(&bgcolour, 1, 1, output);
  fwrite(&pixelratio, 1, 1, output);
 /* colour table is a list of RGB colours */
  struct gifcolourtable {char red; char green; char blue;} __attribute__((packed));
  struct gifcolourtable colourtable[4] = { {0xFF, 0xFF, 0xFF},
                                           {0xFF, 0x00, 0x00},
                                           {0x00, 0x00, 0xFF},
                                           {0x00, 0x00, 0x00}};
  assert(4*3 == fwrite(colourtable, 1, sizeof(colourtable), output));
 /* Optional: Graphics Control Extension. Need this to do animation (has delay) */
  // uint16_t GCEhead = 0xF921; /* Store in little endian, so F9 byte after 0x21 */
  // assert(2 == fwrite(&GCEhead, 2, 1, output));

 /* And now, the image! */
  const char imageconst = 0x2C;
  assert(1 == fwrite(&imageconst, 1, 1, output));
 /* four ints: Image left, image top, width, height */
  uint32_t imagefromorigin = 0; // left and top == 0
  assert(1 == fwrite(&imagefromorigin, 4, 1, output));
  dimensions[0] = 0; dimensions[1] = 0;
  assert(2 == fwrite(dimensions, 2, 2, output));
  flags = 0; /* local colour table, interlace, sort, reserved, size of local colour. All zero. */
  assert(1 == fwrite(&flags, 1, 1, output));
 /* Image Data! */
  // byte:LZW Minimum Code Size
  flags = 2;
  fwrite(&flags, 1, 1, output);
   //many: sub-blocks
     // byte: length of sub-block, in bytes
     // bytes: data, LZW.
     // example dat
     char data[23] = {0x16, 0x8C, 0x2D, 0x99, 0x87, 0x2A, 0x1C, 0xDC, 0x33, 0xA0, 0x02, 0x75,
                   0xEC, 0x95, 0xFA, 0xA8, 0xDE, 0x60, 0x8C, 0x04, 0x91, 0x4C, 0x01};
     fwrite(&data, 23, 1, output);

  //byte: null terminator, no more blocks.
  char a = 0;
  fwrite(&a, 1, 1, output);
 /* Trailer at end of file, done */
  char trailer = 0x3B;
  assert(1 == fwrite(&trailer, 1, 1, output));
  fclose(output);
  output = 0;
  return 0;
}


