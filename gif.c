#include "image.h"

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
  table->size++;
#ifdef DEBUG_TABLE
  printf("Adding %d ", table->size -1);
  int i;
  for(i = 0; i<len; i++){
    printf("%d, ", data[i]);
  }
  putchar('\n');
#endif
  return table->size -1;
}

void writeblock(unsigned char *output, int bitposition, FILE *fd) {
    int bytes = bitposition/8 + (bitposition%8 == 0? 0 : 1); // end is for partial byte
    printf("flushing buffer of length %d\n", bytes);
    fputc(bytes, fd);
    if(1 != fwrite(output, bytes, 1, fd)) {
      puts("Write Fail!");
      exit(-16);
    }
}

int writebits(int value, int bitsize, unsigned char *output, int bitposition, FILE *fd) {
  // Write this code to the output stream:
  //printf("Writing #%d size %d: ", value, bitsize);

  if(bitposition+bitsize > 8*255) {
    writeblock(output, bitposition, fd);
    bitposition = 0;
  }
  while(bitsize > 0) {
    int freebits = 8 - bitposition%8;
    if(freebits == 8) {
      output[bitposition/8] = 0;
    }
    int writebits = freebits > bitsize ? bitsize : freebits ; // min
    output[bitposition/8] |= (value & ((1<<writebits)-1)) << bitposition % 8;
    value = value >> writebits;
    bitsize -= writebits;
    bitposition += writebits;
  }
  return bitposition;
}

void LZW(int colours, char *input, size_t len, FILE* fd) {
  unsigned char *output = malloc(256); // not on stack for valgrind
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
  putc(codesize, fd);
  codesize++; // Because wat, GIF oddness.
  int bitposition = 0;

  const size_t CLEAR = 4;
  const size_t END = 5;

  int pos = 0;
  bitposition = writebits(CLEAR, codesize, output, bitposition, fd);
  /* Encode the input into the code stream */
  while(pos < len) {
    // Find the largest string [...] starting at i that is in the table
    int tlen;
    int this = findprefix(&table, input+pos, len-pos, &tlen); 
    //printf("#%d, ", this);
    bitposition = writebits(this, codesize, output, bitposition, fd);
    if(pos+tlen+1 < len) {
      int new = add(&table, input+pos, tlen + 1);
      if(new == (1<<codesize)) {
        codesize++;
        printf("buffing code size to %d\n", codesize);
        if(codesize >= 12) {
          printf("Resetting code size\n");
          // doesn't seem to work
          exit(-10);
          bitposition = writebits(CLEAR, codesize, output, bitposition, fd);
          codesize = 2;
          table.size= 6;
        }
      }
    }
    pos += tlen;
  }
  bitposition = writebits(END, codesize, output, bitposition,fd );

  writeblock(output, bitposition, fd);

  fputc(0, fd);
  free(output);
}


/*
 * Encode a GIF
 * Takes an image and a buffer to encode into
 * Returns the length of the encoded image
 */
size_t encodeGIF(struct image* image, unsigned char *output, FILE* fd) {
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
    fwrite(output, length, 1 , fd);
    LZW(4, image->data, image->width*image->height, fd);
  }
  fputc(0x3B, fd);
  return length;
}
