/* $Id$

   Dump a binary file of the duration states.
   Helper test data generator.

   Copyright (c) Brett England, 2012

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/

#include "urfdecoder.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "xap.h"

void dump(struct URFContext *ctx, FILE *fd) {
  info("transitions: %d\n", ctx->durStateLen);

  int i;
  for(i=0; i<ctx->durStateLen; i++) {
    uint8_t state = ctx->durState[i] < 0;
    int16_t duration = abs(ctx->durState[i]);
    if(state == 1) duration = -duration;
    //printf("%d", duration);
    fprintf(fd, "%d", duration);
    if(i<ctx->durStateLen-1) {
      //putchar(',');
      fputc(',', fd);
    }
  }
  //putchar('\n');
  fputc('\n',fd);
}

const char *outfile = "urf.in";
int main(int argc, char **argv) {
  FILE *fd;

  setLoglevel(6);

  fd = fopen(outfile, "w");
  if(fd < 0) {
    perror(outfile);
    exit(1);
  }

  // Byrson RS-61
  // A little strange as the OFF can pack into 1 bit/frame.
  // But the ON needs 2 bit/frame so its longer.
  // Probably a capture error but it works so meh.
  dump( initURFContext(NULL, "0101017A0437046001600101271018401014", 0), fd);
  dump( initURFContext(NULL, "010201820436046A015601700436045001700101271018100001820339", 1), fd);


  fclose(fd);
  printf("Created file %s with pulse trains\n", outfile);
  return 0;
}
