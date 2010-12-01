/** iServer for Joggler xAP/TCP gateway
  $Id$

 Copyright (c) Brett England, 2010

 No commercial use.
 No redistribution at profit.
 All derivative work must retain this message and
 acknowledge the work of the original author.
*/

#define YY_MSG_SEQUENCE 1
#define YY_ADD_SOURCE_FILTER 2
#define YY_START_CMD 3
#define YY_START_XAP 4
#define YY_END_CMD 5
#define YY_END_XAP 6
#define YY_IDENT 7
#define YY_LOGIN 8
#define YY_ADD_CLASS_FILTER 9
#define YY_DEL_SOURCE_FILTER 10
#define YY_DEL_CLASS_FILTER 11
#define YY_ADD_SOURCE_FILTER_E 12
#define YY_ADD_CLASS_FILTER_E 13
#define YY_DEL_SOURCE_FILTER_E 14
#define YY_DEL_CLASS_FILTER_E 15

union yy_lval
{
	int   i;
	char  *s;
};

#define YYSTYPE union yy_lval
