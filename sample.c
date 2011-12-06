#include "image.h"

int main() {
  FILE *output = fopen("sample.gif", "w");
  assert(output);

  char imagedata[] = {
  1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 
  1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 
  1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 
  1, 1, 1, 0, 0, 0, 0, 2, 2, 2, 
  1, 1, 1, 0, 0, 0, 0, 2, 2, 2, 
  2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 
  2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 
  2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 
  2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 
  2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 
  };


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
  int outlen = encodeGIF(&image, outbuffer, output);
  fclose(output);
  free(outbuffer);
  return 0;
}
