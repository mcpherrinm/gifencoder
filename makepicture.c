/* Makepicture.c
 *
 * Takes a maze and string (argv || stdin) and converts it to a picture of the
 * maze as it was solved.
 *
 * String format: A series of moves. UDLR (Up Down Left Right)
 *   Todo: Add more commands?
 * Each command probably adds a frame to the animated gif output.
 */
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

  FILE *output = fopen("solved.gif", "wb");
  assert(output);

  /*TODO get len/width from maze */
  int width = 500;
  int height= 500;
  struct colour table[4] =
  { {0xDD, 0xFF, 0xFF},
    {0xFF, 0x00, 0x00},
    {0x00, 0x00, 0xFF},
    {0x00, 0x00, 0x00}
  };

  const int GREY = 0;
  const int RED = 1;
  const int BLUE = 2;
  const int BLACK = 3;

  char *imagedata = malloc(height*width);
  memset(imagedata, GREY, height*width);

  struct image image = {
    4,
    table,
    /*height, width,*/
    500, 100,
    imagedata,
  };

  render(argc == 2? argv[1]: "HI GUYS", BLACK, 10, 5, &image);
  render(argc == 2? argv[1]: "HI GUYS", RED, 20, 5, &image);
  render(argc == 2? argv[1]: "HI GUYS", BLUE, 30, 5, &image);
  unsigned char* outbuffer = malloc(image.width * image.height + 100); //dumb
  int outlen = encodeGIF(&image, outbuffer, output);
  fclose(output);
  free(outbuffer);
//  free(imagedata);
  return 0;
}

