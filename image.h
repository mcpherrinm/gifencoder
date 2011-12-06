#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>

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

size_t encodeGIF(struct image* image, unsigned char *output, FILE *fd);
void render(char *line, int colour, int top, int left, struct image *image);
