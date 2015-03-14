/*  INI file processor
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minIni.h"

char *mybasename(char *path) {
	 char *foo = strrchr(path, '/');
	 if(foo)
	   return ++foo;
	 return path;
}

int iniget(int argc, char *argv[]) {
	 char str[100];
	 long n;
	 if(argc < 4) {
		  fprintf(stderr,"Usage: initget file section key [default]\n");
		  return 1;
	 }
	 char *def = argv[4] == NULL ? "" : argv[4];
	 n = ini_gets(argv[2],argv[3], def, str, sizeof str,argv[1]);
	 if(n>0) {
		  printf("%s",str);
	 }
	 return n;
}

int iniset(int argc, char *argv[]) {
	 long n;
	 if(argc < 5) {
		  fprintf(stderr,"Usage: initset file section key value\n");
		  return 1;
	 }

	 n = ini_puts(argv[2],argv[3],argv[4],argv[1]);
	 return n;
}

int main(int argc, char *argv[]) {
	 char *progname;

	 if(argc > 0) {
		  /* figure which form we're being called as */
	   progname = mybasename(argv[0]);
	   if(strcmp(progname,"iniget") == 0) {
	     return iniget(argc, argv);
	   }
	   if(strcmp(progname,"iniset") == 0) {
	     return iniset(argc, argv);
	   }
	   fprintf(stderr,"INI multi-purpose\n");
	   fprintf(stderr,"Make symlink pointing at this binary with one of the following names:\n"
		   "iniget - read an INI file\n"
		   "iniset - write an INI file\n"
		   );
	   exit(1);
	 }
}
