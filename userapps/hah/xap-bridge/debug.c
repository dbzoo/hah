/* $Id$
   Copyright (c) Brett England, 2010
   
   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.  
*/
#include <stdio.h>
#include <stdarg.h>
#include "bridge.h"

void debug( const int level, const char * format, ... ) {
     if(g_debuglevel > level || level == 0) {
	  va_list ap;
	  va_start(ap, format);
	  vprintf( format, ap );
	  printf("\n");
	  va_end(ap);
     }
}
