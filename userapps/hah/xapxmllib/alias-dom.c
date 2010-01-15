/* $Id$
 */

#include <stdio.h>
#include <libxml/tree.h>
#include <string.h>
#include "alias.h"

extern int g_debuglevel;

static alias_t freeAlias(alias_t alias) 
{
  if(alias == NULL) return;
  alias_t next = alias->next;
  if(alias->xap) free(alias->xap);
  if(alias->select) free(alias->select);
  regfree(alias->selectp);
  free(alias);
  return next;
}

void freeAliases(alias_t alias) 
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
  fprintf(stderr, "%s: %s\n", error, re);
  free(error);
}


static alias_t parseAlias(xmlDocPtr doc, xmlNodePtr cur) 
{
     xmlChar *tmp; // unsigned char
     alias_t alias = (alias_t)malloc(sizeof(alias));

     if(!alias) goto cleanup;

     alias->selectp = (regex_t *)malloc(sizeof(regex_t));
     if(!alias->selectp) goto cleanup;

     tmp = xmlGetProp(cur,"select");
     if(!tmp) goto cleanup;
     alias->select = strdup(tmp);
     xmlFree(tmp);
     if(!alias->select) goto cleanup;
     
     int ret = regcomp(alias->selectp, alias->select, REG_ICASE);
     if(ret) {
	  if(g_debuglevel) regex_error(ret, alias->selectp, alias->select);    
	  goto cleanup;
     }

     tmp = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
     if(!tmp) goto cleanup;
     alias->xap = strdup(tmp);
     xmlFree(tmp);
     if(!alias->xap) goto cleanup;

     alias->next = NULL;
     return alias;

cleanup:
     if(g_debuglevel) perror("parseAlias");
     if(alias && alias->select) free(alias->select);
     if(alias && alias->selectp) free(alias->selectp);
     if(alias && alias->xap) free(alias->xap);
     if(alias) free(alias);
     return NULL;
}

/* Supplied with a file parse the following XML and return 
   a linked list of aliases.

   <gcal>
   <alias select="regexp">xap message</alias>
   <alias select="regexp">xap message</alias>
   </gcal>
*/
alias_t parseAliasDoc(char *docname) 
{
  xmlDocPtr doc;
  xmlNodePtr cur;
  alias_t alias_list = NULL;
  
  doc = xmlParseFile(docname);
  if(doc == NULL) {
    if(g_debuglevel) fprintf(stderr,"Document not parsed successfully.\n");
    return NULL;
  }

  cur = xmlDocGetRootElement(doc);
  if (cur == NULL) {
    if(g_debuglevel) fprintf(stderr,"Empty document.\n");
    goto error;
  }

  if(strcmp(cur->name, "gcal")) {
    if(g_debuglevel) fprintf(stderr,"Document of the wrong type, root node != gcal");
    goto error;
  }
  cur = cur->xmlChildrenNode;

  while (cur != NULL) {
    if(strcmp(cur->name, "alias") == 0) {
      alias_t elem = parseAlias(doc, cur);
//      dumpAliases(elem);
      if (elem) {
	if (alias_list == NULL) {
	  alias_list = elem;
	} else {
	  elem->next = alias_list;
	  alias_list = elem;
	}
      }
    }
    cur = cur->next;
  }

 error:
  xmlFreeDoc(doc);
  return alias_list;
}

void dumpAliases(alias_t alias) 
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
char *matchAlias(alias_t alias, char *seek) 
{
  int ret;
  char out[1500];
  char pat[1500];
  
  if(g_debuglevel >= 3) printf("Matching command: %s\n", seek);
  while(alias) {
    if(g_debuglevel >= 3) printf("test pattern: %s\n", alias->select);
    strlcpy(out, seek, sizeof(out));
    strlcpy(pat, alias->xap, sizeof(out));
    ret = rreplace(out, sizeof(out), alias->selectp, pat);
    if(ret) {
      if(ret == REG_NOMATCH) break;
      if(g_debuglevel) regex_error(ret, alias->selectp, alias->select);
    } else {
      return strdup(out);
    }
    alias = alias->next;
  }
  return NULL;
}
