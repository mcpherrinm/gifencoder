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

size_t bitstream(int codesize, char *codebuf, size_t codes, unsigned char* output);


struct lztable {
  int size;
  struct {char *data; size_t len;} d[4096];
};

/* Inefficient. Better data structure could help */
int findprefix(struct lztable *table, char *where, int len, int *rvlen) {
  int t;
  int bestlen = 0;
  int bestcode = -1;
  for(t = 0; t < table->size; t++) {
    if(t == 4 || t == 5) continue;
    int tl = table->d[t].len;
    char *d = table->d[t].data;
    if(bestlen < tl && len >= tl
       && 0 == memcmp(where, d, tl)) {
      bestcode = t;
      bestlen = tl;
    }
  }
  *rvlen = bestlen;
  return bestcode;
}

/* Adds entry to table */
int add(struct lztable *table, char *data, int len) {
  if(table->size > 4095) {
    printf("Table size blowout; reset not implemented\n");
    exit(-3);
  }
  table->d[table->size].len = len;
  table->d[table->size].data = data;
  printf("Adding %d ", table->size);
  table->size++;
  int i;
  for(i = 0; i<len; i++){
    printf("%d, ", data[i]);
  }
  putchar('\n');
  return table->size -1;
}

int writebits(int value, int bitsize, unsigned char *output, int bitposition) {
  // Write this code to the output stream:
  int freebits = 8 - bitposition%8;
  printf("Writing #%d size %d\n", value, bitsize);
  if(freebits >= bitsize) {
    if(freebits == 8) {
      output[bitposition/8] = 0;
    }
    output[bitposition/8] |= value << bitposition % 8;
  } else {
    // split into two bytes
    unsigned char mask = (1<<freebits) - 1;
    output[bitposition/8] |= (value &  mask) << bitposition % 8;
    output[bitposition/8 + 1] = value >> freebits;
  }
  return bitposition + bitsize;
}

size_t LZW(int colours, char *input, size_t len, unsigned char *output) {
  char init_colours[] = {0,1,2,3};
  int j = 0;
  struct lztable table;
  table.size=6;
  for(;j < 4; j++) {
    table.d[j].data = &(init_colours[j]);
    table.d[j].len  = 1;
    putchar('k');
  }
  putchar('\n');

  int codesize = 2;
  output[0] = codesize;
  codesize++; // Because wat, GIF oddness.
  output[1] = 0xFF; // block size goes here
  int bitposition = 16;

  const size_t CLEAR = 4;
  const size_t END = 5;

  int pos = 0;
  bitposition = writebits(CLEAR, codesize, output, bitposition);
  /* Encode the input into the code stream */
  while(pos < len) {
    // Find the largest string [...] starting at i that is in the table
    int tlen;
    int this = findprefix(&table, input+pos, len-pos, &tlen); 
    bitposition = writebits(this, codesize, output, bitposition);
    if(pos+tlen+1 < len) {
      int new = add(&table, input+pos, tlen + 1);
      if(new == (1<<codesize)-1) {
        codesize++;
        printf("buffing code size to %d\n", codesize);
      }
    }
    pos += tlen;
  }
  bitposition = writebits(END, codesize, output, bitposition);

  int last = bitposition/8 + 1;
  output[1] = last;
  output[last++] = 0;
  return last;
}




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

  const char sampledata[100] = {
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
    10, 10,
    imagedata,
  };

  unsigned char* outbuffer = malloc(image.width * image.height + 100); //dumb
  int outlen = encodeGIF(&image, outbuffer);
  fwrite(outbuffer, outlen, 1, output);
  fclose(output);
  free(outbuffer);
  return 0;
}

