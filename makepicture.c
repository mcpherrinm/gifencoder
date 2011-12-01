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

/* GIF image header and logical screen descriptor */
struct GIFheader {
  char HEAD[6];
  uint16_t width;
  uint16_t height;
  /*  0b10010001 flags with image settings
   *    |\|/|\|/
   *    | | | \-- (3 bits) Size of global Color Table
   *    | | \---- (1 bit)  Sort flag (something about colour sorting?!)
   *    | \------ (3 bits) Colour resolution: 2^(n+1), so here 4.
   *    \-------- (1 bit)  Global Colour table flag; whether there is a colour table
   */
   char packed;
   char bgcolour;
   char pixelaspectratio;
} __attribute__((packed));

size_t LZW(int colours, char *input, size_t len, char *output) {
  struct pair {short code; char *data; size_t len;};
  char init_colours[] = {0,1,2,3};
#define I(i) {i, &(init_colours[i])}
  struct pair table[32] = { I(0), I(1), I(2), I(3), {4, 0}, {5, 0} };
  int ts = 6;
  (void)ts;
  (void)output;
  size_t clear = 4;
  size_t END = 5;

  char *codes = malloc(sizeof(len+3)); // uncompressed length + two start bytes + end byte
  codes[0] = 2; // bits per pixel
  codes[1] = table[clear].code;
  int i = 2;
  for(;i < len; i++) {
    // uh, literal codes are cool guys. Let's not compress!
    codes[i] = input[i];
  }

  codes[i] = END;
  char desiredoutdata[] = {0x02, 0x16, 0x8C, 0x2D, 0x99, 0x87, 0x2A, 0x1C, 0xDC, 0x33, 0xA0, 0x02,
                             0x75, 0xEC, 0x95, 0xFA, 0xA8, 0xDE, 0x60, 0x8C, 0x04, 0x91, 0x4C, 0x01, 0x0};
  memcpy(output, desiredoutdata, sizeof(desiredoutdata));
  return sizeof(desiredoutdata);
};

/*
 * Encode a GIF
 * Takes an image and a buffer to encode into
 * Returns the length of the encoded image
 */
size_t encodeGIF(struct image* image, char *output) {
  const uint16_t delaytime = 0;

  size_t length = 0;

  /*Header*/
  memcpy(output, "GIF89a", 6);
  ((struct GIFheader *)output)->width = image->width;
  ((struct GIFheader *)output)->height = image->height;
  ((struct GIFheader *)output)->packed = 0x91; // todo; make this depend on image colour table
  ((struct GIFheader *)output)->bgcolour = 0; // first colour is background. Usually white.
  ((struct GIFheader *)output)->pixelaspectratio = 0;
  length += sizeof(struct GIFheader);

  /*Global Colour Table*/
  memcpy(output+length, image->colourtable, sizeof(struct colour) * image->colours);
  length += sizeof(struct colour) * image->colours;

  int i, frames = 1; // this should be a param
  for(i = 0; i < frames; i++) {
    /* Graphics Control Extension*/
    output[length++] = 0x21; /* Extension Introducer */
    output[length++] = 0xF9; /* Graphics Control Label */
    output[length++] =    4; /* Not really sure. Size of something?*/
    output[length++] =    0; /* Flags. lowest bit is transparent flag */
    *(uint16_t*)(output+length) = delaytime;
    length+=2;
    output[length++] = 0; /*What colour is transparent*/
    output[length++] = 0;

    /* Image Descriptor */
    output[length++] = 0x2C; /* Image Seperator */

    /* Image Left */
    *(uint16_t*)(output+length) = 0;
    length+=2;

    /* Image Top */
    *(uint16_t*)(output+length) = 0;
    length+=2;

    /* Image Width */
    *(uint16_t*)(output+length) = image->width;
    length+=2;

    /* Image Height */
    *(uint16_t*)(output+length) = image->height;
    length+=2;

    /* More flags: Local colour table; interlacing */
    output[length++] = 0;

    length += LZW(4, image->data, image->width*image->height, output+length);
  }
  output[length++] = 0x3B; /* Trailer, end of file */
  return length;
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

  char imagedata[100] = {
    1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
    1, 1, 1, 0, 0, 0, 0, 2, 2, 2,
    1, 1, 1, 0, 0, 0, 0, 2, 2, 2,
    2, 2, 2, 0, 0, 0, 0, 1, 1, 1,
    2, 2, 2, 0, 0, 0, 0, 1, 1, 1,
    2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 1, 1, 1, 1, 1 };

  struct colour table[4] =
    {{0xFF, 0xFF, 0xFF},
     {0xFF, 0x00, 0x00},
     {0x00, 0x00, 0xFF},
     {0x00, 0x00, 0x00}};

  struct image image = {
    4,
    table,
    10, 10,
    imagedata,
  };
  char outbuffer[1024];
  int outlen = encodeGIF(&image, outbuffer);
  fwrite(outbuffer, outlen, 1, output);
  fclose(output);
  return 0;
}

