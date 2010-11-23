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
"<flt+>"	{ return(YY_SOURCE_FILTER); }
"<cflt+>"	{ return(YY_CLASS_FILTER); }
"<cmd>"		{ return(YY_START_CMD); }
"<xap>"		{ return(YY_START_XAP); }
"</cmd>"	{ return(YY_END_CMD); }
"</xap>"	{ return(YY_END_XAP); }
"log<code>"     { return(YY_LOGIN); }
[^</\/]*	{/* ANY char using </ as a terminal */
                 yylval.s = yytext; return YY_IDENT;
                }
%%

int yywrap(void)
{
	return(1);
}
