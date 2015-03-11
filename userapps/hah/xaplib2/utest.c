/* Tests for the xap parser
 */
#include <stdio.h>
#include <string.h>
#include "xap.h"

int main(int argc, char **argv) {
  xAPFrame f;

  strcpy(f.dataPacket,"Section\n{\nHello=WORLD\nHi=\nWorlD= hello\nthere=\n}\n");
  f.len = strlen(f.dataPacket);
  printf("%s\n", f.dataPacket);

  parseMsgF(&f);
  xapLowerMessageF(&f);
  
  char output[1024];
  parsedMsgToRawF(&f, output, sizeof(output));
  printf("%s\n", output);
}
