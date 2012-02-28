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

void dump(struct URFContext *ctx, int fd) {
  printf("transitions: %d\n", ctx->durStateLen);
  printf("%s %s\n", ctx->xAPTarget, ctx->bscState);

  write(fd, ctx->durState, ctx->durStateLen * sizeof(int));

  int i;
  for(i=0; i<ctx->durStateLen; i++) {
    uint8_t state = ctx->durState[i] < 0;
    uint16_t duration = abs(ctx->durState[i]);
    printf("[%d,%d] ", state, duration);
  }
  printf("\n\n");
}

int main(int argc, char **argv) {
  int fd;

  setLoglevel(6);

  fd = creat("urf.in", 0644);
  if(fd < 0) {
    perror("urf.in");
    exit(1);
  }

  dump( initURFContext("01010177046504650177060127101900001400","dbzoo.livebox.controller:rf.1","off"), fd);
  dump( initURFContext("01010177046504650177060127101900001500","dbzoo.livebox.controller:rf.1","on"), fd);
  dump( initURFContext("01010177046504650177060127101900001500","dbzoo.livebox.controller:rf.1","on"), fd);
  dump( initURFContext("01010177046504650177060127101900001500","dbzoo.livebox.controller:rf.1","on"), fd);

  close(fd);
  return 0;
}
