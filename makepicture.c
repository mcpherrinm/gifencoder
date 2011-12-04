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

/*
 *  
 */
size_t LZW(int colours, char *input, size_t len, unsigned char *output) {

  int i = 0;
  unsigned char *codes = malloc(sizeof(len+3)); // uncompressed length + two start bytes + end byte
  {

  struct entry {char *data; size_t len;};
  char init_colours[] = {0,1,2,3};
#define I(i) {&(init_colours[i]), 1}
  /* This table is big enough: After 0xFFF, need to <cc> and restart */
  struct entry table[4096] = { I(0), I(1), I(2), I(3), {0, 1}, {0, 1} };
  int ts = 6;

  size_t clear = 4;
  size_t END = 5;

  int j = 0;
  codes[i++] = clear;
  /* Encode the input into the code stream */
  while(j < len) {
    // Find the largest string [...] starting at i that is in the table
    int pflen = 1;
    int t;
    int best = -1;
    int bestlen = 0;
    // Inefficient:
    for(t = 0; t < ts; t++) {
      if(pflen+j > len) break;
      if(t == 4 || t == 5) continue;
      if(pflen == table[t].len && 0 == memcmp(input+j, table[t].data, pflen)) {
        best = t;
        bestlen = pflen;
        t = 0;
        pflen++;
      }
    }
    /* Add [...]K (1 longer than match) to the table */
    table[ts].data = input+j;
    table[ts].len  = pflen;
    /////
    printf("#%d from %d\t", ts, j);
    int ii = 0;
    for(ii = 0; ii < table[ts].len; ii++) {
      printf("%d, ", table[ts].data[ii]);
    }
    putchar('\n');
    /////
    ts++;
    j += bestlen;
    codes[i++] = best;
  }

  codes[i++] = END;

  }

  {
  int bitsperpixel = 0;
  if(colours == 4) {
    bitsperpixel=2;
  }
  int codesize = 2; // max(2, bitsperpixel) -- for > 4 colour support.
  output[0] = codesize;
  codesize++; // Because wat, GIF oddness.
  output[1] = 0xFF; // block size goes here
  int bitposition = 16;
  int j = 0;
  for(;j < i; j++){
    if(codes[j] == (1<<codesize) - 1) {
      codesize++;
      printf("buffing code size to %d\n", codesize);
    }
    // Write this code to the output stream:
    int freebits = 8 - bitposition%8;
    if(freebits >= codesize) {
      if(freebits == 8) {
        output[bitposition/8] = 0;
      }
      printf("Writing %dth code %d to byte %d shifted %d\n", j, codes[j], bitposition/8, bitposition % 8);
      output[bitposition/8] |= codes[j] << bitposition % 8;
    } else {
      // split into two bytes
      printf("Writing %dth code %d to bytes %d, %d shifted %d\n", j, codes[j], bitposition/8, bitposition/8 +1, bitposition % 8);
      unsigned char mask = (1<<freebits) - 1;
      output[bitposition/8] |= (codes[j] &  mask) << bitposition % 8;
      output[bitposition/8 + 1] = codes[j] >> freebits;
    }
    bitposition += codesize;
  }
  int last = bitposition/8 + 1;
  output[1] = last;
  output[last++] = 0;
  return last;
  }
};

/*
 * Encode a GIF
 * Takes an image and a buffer to encode into
 * Returns the length of the encoded image
 */
size_t encodeGIF(struct image* image, unsigned char *output) {
  const uint16_t delaytime = 0;

  size_t length = 0;

  /*Header*/
  memcpy(output, "GIF89a", 6);
  ((struct GIFheader *)output)->width = image->width;
  ((struct GIFheader *)output)->height = image->height;
  ((struct GIFheader *)output)->packed = 0x91; // todo; make this depend on image colour table
  ((struct GIFheader *)output)->bgcolour = 0; // first colour is background.
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

  char imagedata[400] = {
    1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
    1, 1, 1, 0, 0, 0, 0, 2, 2, 2,
    1, 1, 1, 0, 0, 0, 0, 2, 2, 2,
    2, 2, 2, 0, 0, 0, 0, 1, 1, 1,
    2, 2, 2, 0, 0, 0, 0, 1, 1, 1,
    2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 1, 1, 1, 1, 1
    };

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
  unsigned char outbuffer[4000]; // TODO size pessimistically
  int outlen = encodeGIF(&image, outbuffer);
  fwrite(outbuffer, outlen, 1, output);
  fclose(output);
  return 0;
}

