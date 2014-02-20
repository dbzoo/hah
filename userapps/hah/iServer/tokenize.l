%{
/* iServer protocol tokenizer
   $Id$

  Copyright (c) Brett England, 2010

  No commercial use.
  No redistribution at profit.
  All derivative work must retain this message and
  acknowledge the work of the original author.

  http://dinosaur.complilertools.net/
*/
#include "tokenize.h"
#include <stdlib.h>

extern YYSTYPE yylval;
%}

%%
"[["[0-9]+"]]"	{ yylval.i = atoi(yytext+2); return(YY_MSG_SEQUENCE); }
"<flt+>"	{ return(YY_ADD_SOURCE_FILTER); }
"</flt+>"	{ return(YY_ADD_SOURCE_FILTER_E); }
"<flt->"	{ return(YY_DEL_SOURCE_FILTER); }
"</flt->"	{ return(YY_DEL_SOURCE_FILTER_E); }
"<cflt+>"	{ return(YY_ADD_CLASS_FILTER); }
"</cflt+>"	{ return(YY_ADD_CLASS_FILTER_E); }
"<cflt->"	{ return(YY_DEL_CLASS_FILTER); }
"</cflt->"	{ return(YY_DEL_CLASS_FILTER_E); }
"<cmd>"		{ return(YY_START_CMD); }
"</cmd>"	{ return(YY_END_CMD); }
"<"[xX][aA][pP]">"      { return(YY_START_XAP); }
"</"[xX][aA][pP]">"	{ return(YY_END_XAP); }
"log<code>"     { return(YY_LOGIN); }
.|\n	{/* ANY unmatched character */
            yylval.s = yytext; return YY_IDENT;
        }
%%

int yywrap(void)
{
	return(1);
}
