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

struct colour {
  char red;
  char green;
  char blue;
} __attribute__((packed));

/* Represents an uncompressed image */
struct image {
  /* How many colours are in the image*/
  int colours;
  /* What the colours are */
  struct colour *colourtable;
  int width;
  int height;
  char *data;
};


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
  //printf("Commands: '''%s''', Maze: '''%s'''\n", commands, maze);
#endif
 /* Start Writing GIF. 6 byte header & version is GIF89a */
  FILE *output = fopen("solved.gif", "w");
  assert(output);

  const char sampledata[400] = {
  1, 1, 1, 1, 1, 2, 2, 2, 2, 2,  1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
  1, 1, 1, 1, 1, 2, 2, 2, 2, 2,  1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
  1, 1, 1, 1, 1, 2, 2, 2, 2, 2,  1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
  1, 1, 1, 0, 0, 0, 0, 2, 2, 2,  1, 1, 1, 0, 0, 0, 0, 2, 2, 2,
  1, 1, 1, 0, 0, 0, 0, 2, 2, 2,  1, 1, 1, 0, 0, 0, 0, 2, 2, 2,
  2, 2, 2, 0, 0, 0, 0, 1, 1, 1,  2, 2, 2, 0, 0, 0, 0, 1, 1, 1,
  2, 2, 2, 0, 0, 0, 0, 1, 1, 1,  2, 2, 2, 0, 0, 0, 0, 1, 1, 1,
  2, 2, 2, 2, 2, 1, 1, 1, 1, 1,  2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 1, 1, 1, 1, 1,  2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 1, 1, 1, 1, 1,  2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 2, 2, 2, 2, 2,  1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
  1, 1, 1, 1, 1, 2, 2, 2, 2, 2,  1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
  1, 1, 1, 1, 1, 2, 2, 2, 2, 2,  1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
  1, 1, 1, 0, 0, 0, 0, 2, 2, 2,  1, 1, 1, 0, 0, 0, 0, 2, 2, 2,
  1, 1, 1, 0, 0, 0, 0, 2, 2, 2,  1, 1, 1, 0, 0, 0, 0, 2, 2, 2,
  2, 2, 2, 0, 0, 0, 0, 1, 1, 1,  2, 2, 2, 0, 0, 0, 0, 1, 1, 1,
  2, 2, 2, 0, 0, 0, 0, 1, 1, 1,  2, 2, 2, 0, 0, 0, 0, 1, 1, 1,
  2, 2, 2, 2, 2, 1, 1, 1, 1, 1,  2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 1, 1, 1, 1, 1,  2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 1, 1, 1, 1, 1,  2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
  };

  char *imagedata = malloc(sizeof(sampledata));

  memcpy(imagedata, sampledata, sizeof(sampledata));

  struct colour table[4] =
  { {0xFF, 0xFF, 0xFF},
    {0xFF, 0x00, 0x00},
    {0x00, 0x00, 0xFF},
    {0x00, 0x00, 0x00}
  };

  struct image image = {
    4,
    table,
    20, 20,
    imagedata,
  };

  unsigned char* outbuffer = malloc(image.width * image.height + 100); //dumb
  int outlen = encodeGIF(&image, outbuffer);
  fwrite(outbuffer, outlen, 1, output);
  fclose(output);
  free(outbuffer);
  return 0;
}

