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
#include <string.h>
#include "image.h"

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
#if 0
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
#endif

  FILE *output = fopen("solved.gif", "w");
  assert(output);


  char *imagedata = malloc(500*500);
  memset(imagedata, 0, 500*500);


  struct colour table[4] =
  { {0xDD, 0xDD, 0xDD},
    {0xFF, 0x00, 0x00},
    {0x00, 0x00, 0xFF},
    {0x00, 0x00, 0x00}
  };

  struct image image = {
    4,
    table,
    160, 25,
    imagedata,
  };

  render("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 3,10, 0, &image);
  render("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 1,18, 0, &image);
  unsigned char* outbuffer = malloc(image.width * image.height + 100); //dumb
  int outlen = encodeGIF(&image, outbuffer);
  fwrite(outbuffer, outlen, 1, output);
  fclose(output);
  free(outbuffer);
  return 0;
}

