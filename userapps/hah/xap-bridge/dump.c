/* Module DUMP.C

   begun 26 11 88 crc last modified 3 3 89 crc Adapted to Microsoft C
   17 10 89 crc, and to Linux and ANSI C 2003 05 15.

   Time-stamp: <2003-09-17 13:42:08 ccurley dump.c>

   Copyright 1988 through the last date of modification, Charles Curley.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA

   You can also contact the Free Software Foundation at
   http://www.fsf.org/

   For programmers who like to C what they're doing, and are down in
   the DUMPs about C's lack of interactivity.

   Add this file to your make file if you want to call to either
   dump() or ldump() , below. Each takes a pointer and a count.  It is
   up to the user to set them up correctly. When you are satisfied
   that your code runs correctly, comment out the references to the
   functions, and you are all set to run your code. Leave the
   commented out references in the source so that you can readily
   restore them should you need to modify your program later. It is
   also up to the user to turn the cursor on and off, if he wants to
   get fancy. The ASCII field prints data as ASCII characters,
   regardless of bit 7.

   To compile: gcc -o dump dump.c -DTEST -ansi -pedantic -Wall

*/

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define MIN(x , y)  (((x) < (y)) ? (x) : (y))
/*	#define TEST = 1 */

#ifdef TEST

void dump(char *ptr, unsigned int ct);

int
main() {
  char *msg = "This is a test. Foo. Bar.\n";

  dump((char *)msg,(1+strlen(msg)));
  printf(msg);

  return (0);
}

#endif


/* dump a line (16 bytes) of memory, starting at pointer ptr for len
   bytes */

void
ldump(char *ptr, unsigned int len) {
  if ( len ) {
    int  i;
    char c;
    len=MIN(16,len);
    /*    printf("\nStart is %10x, Count is %5x, End is %10x",ptr,len,ptr+len); */
    printf("%10p ",ptr);

    for( i = 0 ; i < len ; i++ )  /* Print the hex dump for len
                                     chars. */
      printf("%3x",(*(ptr+i)&0xff));

    for( i = len ; i < 16 ; i++ ) /* Pad out the dump field to the
                                     ASCII field. */
      printf("   ");

    printf(" ");
    for( i = 0 ; i < len ; i++ ) { /* Now print the ASCII field. */
      c=0x7f&(*(ptr+i));      /* Mask out bit 7 */

      if (!(isprint(c))) {    /* If not printable */
        printf(".");          /* print a dot */
      } else {
        printf("%c",c);  }    /* else display it */
    }
  }
  printf("\n");
}

/* Print out a header for a hex dump starting at address st. Each
   entry shows the least significant nybble of the address for that
   column. */

void
head(long st) {
  int i;
  printf("   addr    ");
  for ( i = st&0xf ; i < (st&0xf)+0x10 ; i++ )
    printf("%3x",(i&0x0f));
  printf(" | 7 bit ascii. |\n");
}


/* Dump a region of memory, starting at ptr for ct bytes */

void
dump(char *ptr, unsigned int ct) {
  if ( ct ) {
    int i;

    /*  preliminary info for user's benefit. */
    printf("Start is %10p, Count is %5x, End is %10p\n",
           ptr, ct, ptr + ct);
    head((long)ptr);
    for ( i = 0 ; i <= ct ; i = i+16 , ptr = ptr+16 )
      ldump(ptr,(MIN(16,(ct-i))));
  }
  printf("\n");
}
