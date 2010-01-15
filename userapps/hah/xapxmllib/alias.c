/* $Id$
 *
 * The alias XML file is very simple so we will use a lightweight
 * SAX parser to turn this into a linked-list of aliases.
 */

#include <stdio.h>
#include <libxml/parser.h>
#include <string.h>
#include "alias.h"
#include "xapdef.h"

extern int g_debuglevel;

typedef struct _ParserState {
     int loadXap;
     alias_t *alias_list;
} ParserState;

static alias_t *freeAlias(alias_t *alias) 
{
  if(alias == NULL) return;
  alias_t *next = alias->next;
  if(alias->xap) free(alias->xap);
  if(alias->select) free(alias->select);
  regfree(alias->selectp);
  free(alias);
  return next;
}

void freeAliases(alias_t *alias) 
{
  while(alias) {
    alias = freeAlias(alias);
  }
}

static void regex_error(int err, regex_t *compiled, const char *re) 
{
  int length = regerror(err, compiled, NULL, 0);
  char *error = (char *)malloc(length + 1);
  if(error == NULL && g_debuglevel) {
       perror("regex_error");
       return;
  }
  regerror(err, compiled, error, length);
  printf("%s: %s\n", error, re);
  free(error);
}

/* SAX element (TAG) callback
 */
static void startElementCB(void *ctx, const xmlChar *name, const xmlChar **atts)
{
     ParserState *state = (ParserState *)ctx;

     if(strcmp("alias",name)) return;
     if(strcmp("select",atts[0])) return;
     
     alias_t *alias = (alias_t *)malloc(sizeof(alias_t));
     if(!alias) return;

     alias->next = NULL;
     alias->select = strdup(atts[1]);
     if(alias->select == NULL) goto cleanup;
     alias->selectp = (regex_t *)malloc(sizeof(regex_t));
     if(alias->selectp == NULL) goto cleanup;
     int ret = regcomp(alias->selectp, alias->select, REG_ICASE);
     if(ret) {
	  if(g_debuglevel) regex_error(ret, alias->selectp, alias->select);    
	  goto cleanup;
     }

     // Add the alias to our list.
     if (state->alias_list == NULL) {
	  state->alias_list = alias;
     } else {
	  alias->next = state->alias_list;
	  state->alias_list = alias;
     }
     state->loadXap = 1; // the next CDATA is our XAP data.

     return;

cleanup:
     freeAlias(alias);
}

/* SAX cdata callback
 */
static void cdataBlockCB(void *ctx, const xmlChar *value, int len)
{
     ParserState *state = (ParserState *)ctx;
     if(state->loadXap) {
	  if(value && *value)
	       // The xap-header in the ALIAS file can only specify as much as
	       // we allow in the CALENDER.  All other fields will be auto populated.
	       state->alias_list->xap = normalize_xap(value);
	  state->loadXap = 0;
     }
}

/* Supplied with a file parse the following XML and return 
   a linked list of aliases.

   <gcal>
   <alias select="regexp">xap message</alias>
   <alias select="regexp">xap message</alias>
   </gcal>
*/
alias_t *parseAliasDoc(char *docname) 
{
     ParserState saxState;
     xmlSAXHandler handler;

     memset(&saxState, 0, sizeof(saxState));
     memset(&handler, 0, sizeof(handler));

     handler.startElement = startElementCB;
     handler.characters = cdataBlockCB;
     
     if(xmlSAXUserParseFile(&handler, &saxState, docname) < 0) {
	  if(g_debuglevel) printf("Document not parsed successfully.\n");
	  return NULL;
     }

     // The struct is free'd from the stack but the memory for the
     // alias linked list will remain allocated on the heap.
     // Caller is responsible for freeing this.
     return saxState.alias_list;
}

void dumpAliases(alias_t *alias) 
{
  while(alias) {
       if(alias->select) printf("ALIAS: %s\n", alias->select);
       if(alias->xap) printf("XAP: %s\n", alias->xap);
       alias = alias->next;
  }
}

/**
 * Example:
 *   Regex re: "rf \([0-9]\) \(off\|on\)"
 *   Replace pattern rp: "XAP: rf $1 is $2"
 *   Buf: "rf 1 on"
 * RETURN
 *   Buf: "XAP: rf 1 is on"
 * 
 * Adapted from: http://www.daniweb.com/code/snippet216955.html
 */
static int rreplace (char *buf, int size, regex_t *re, char *rp)
{
  char *pos;
  int sub, so, n;
  regmatch_t pmatch [10]; /* regoff_t is int so size is int */

  int ret = regexec (re, buf, 10, pmatch, 0);
  if (ret) return ret;
  for (pos = rp; *pos; pos++)
    if (*pos == '$' && *(pos + 1) > '0' && *(pos + 1) <= '9') {
      so = pmatch [*(pos + 1) - 48].rm_so;
      n = pmatch [*(pos + 1) - 48].rm_eo - so;
      if (so < 0 || strlen (rp) + n - 1 > size) return REG_ESPACE;
      memmove (pos + n, pos + 2, strlen (pos) - 1);
      memmove (pos, buf + so, n);
      pos = pos + n - 2;
    }
  sub = pmatch [1].rm_so; /* no repeated replace when sub >= 0 */
  for (pos = buf; !regexec (re, pos, 1, pmatch, 0); ) {
    n = pmatch [0].rm_eo - pmatch [0].rm_so;
    pos += pmatch [0].rm_so;
    if (strlen (buf) - n + strlen (rp) > size) return REG_ESPACE;
    memmove (pos + strlen (rp), pos + n, strlen (pos) - n + 1);
    memmove (pos, rp, strlen (rp));
    pos += strlen (rp);
    if (sub >= 0) break;
  }
  return 0;
}

/**
 * Supplied with a list of aliases examine them all looking for
 * a regular expression that matches the "seek" command.
 * The SEEK command is normally the google calendar event "TITLE".
 *
 * When one it found return the regexp substituted XAP payload
 * 
 * Caller is responsible for free the memory.
 */
char *matchAlias(alias_t *alias, char *seek) 
{
  int ret;
  char out[1500];
  char pat[1500];
  
  if(g_debuglevel >= 3) printf("Matching command: '%s'\n", seek);
  while(alias) {
    if(g_debuglevel >= 3) printf("test pattern: '%s'\n", alias->select);
    strcpy(out, seek);
    strcpy(pat, alias->xap);
    ret = rreplace(out, sizeof(out), alias->selectp, pat);
    if(ret) {
	 if(ret == REG_NOMATCH) {
	      if(g_debuglevel >= 3) printf("No match\n");
	      break;
	 }
      if(g_debuglevel) regex_error(ret, alias->selectp, alias->select);
    } else {
	 if(g_debuglevel >= 3) printf("Match\n");
	 return strdup(out);
    }
    alias = alias->next;
  }
  return NULL;
}
