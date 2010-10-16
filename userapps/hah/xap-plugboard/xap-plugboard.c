/* $Id$
 *
 * A script engine to allow programmatic control over events and actions.
 */
#ifdef IDENT
#ident "@(#) $Id$"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/select.h>
#include <time.h>

#include "xapdef.h"
#include "xapGlobals.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "msg_cache.h"
#include "debug.h"

const char* XAP_ME = "dbzoo";
const char* XAP_SOURCE; 
const char* inifile = "/etc/xap-livebox.ini";
const char* XAP_GUID;
const char* XAP_DEFAULT_INSTANCE = "Plugboard";
static char scriptdir[64];

enum Type {SOURCE, TARGET, CLASS};
const char *type_s[] = {"source","target","class"};

typedef struct _callback {
     enum Type type;
     char *pattern;
     char *func;
} Callback;

typedef struct _timer {
     char *name; // Timer name
     time_t time;   // Event time (counts down to 0)
     char *func; // LUA callback function
} Timer;

typedef struct _xaplet {
     char *filename;
     lua_State *state;
     Callback **callbacks;
     Timer **timers;
     int timer_size;
     int callback_size;     
} Xaplet;

static Xaplet **xaplet;
static int xaplet_size = 0;
static xapEntry *receivedMsg;

// LUA Stack dump, debug helper
void stackDump(lua_State* L)  
{  
    int i;  
    int top = lua_gettop(L);  
  
    printf("totaL in stack %d\n",top);  
  
    for (i = 1; i <= top; i++)  
    {  /* repeat for each LeveL */  
	 int t = lua_type(L, i);  
	 switch (t) {  
	 case LUA_TSTRING:  /* strings */  
	      printf("string: '%s'\n", lua_tostring(L, i));  
	      break;  
	 case LUA_TBOOLEAN:  /* booLeans */  
	      printf("boolean %s\n",lua_toboolean(L, i) ? "true" : "false");  
	      break;  
	 case LUA_TNUMBER:  /* numbers */  
	      printf("number: %g\n", lua_tonumber(L, i));  
	      break;  
	 defauLt:  /* other values */  
	      printf("%s\n", lua_typename(L, t));  
	      break;  
	 }  
	 printf("  ");  /* put a separator */  
    }  
    printf("\n");  /* end the Listing */  
}

void runLuaCallback(Xaplet *c, Callback *cb, char *xapPattern) {
     if(g_debuglevel > 7) printf("check %s -> %s\n", cb->pattern, xapPattern);
     if(!xap_compare(cb->pattern, xapPattern)) return; // No match?
     
     if(g_debuglevel > 3) printf("run %s callback '%s'\n", c->filename, cb->func);
     lua_getglobal(c->state, cb->func);

     // Do I want to pass anything into the function or expect anything in return?
     lua_call( c->state, 0, 0 );
}

/* As an "lua_register" call cannot pass in userdata we use its STATE to find the
   associated lua script.  A bit ugly?!
*/
Xaplet *findXapletFromState(lua_State *L) {
     int i;
     for(i=0; i<xaplet_size; i++) {
	  if(L == xaplet[i]->state) return xaplet[i];
     }
     return NULL;
}

/********************* LUA C callback functions *********************
 *
 * LUA will register a callback like this:
 *
 * register_source('dbzoo.livebox.Controller:relay.1', "dosource")
 * 
 * We add this (pattern, func) to the scripts list of things that its intersted in.
 */
void addCallback(lua_State *L, const char *pattern, const char *func, enum Type type) { 
     if(g_debuglevel) printf("register_%s('%s','%s')\n", type_s[type], pattern, func);
     if(pattern == NULL || func == NULL) return;

     Xaplet *c = findXapletFromState(L);
     if(c == NULL) return;

     c->callback_size ++;
     c->callbacks = (Callback **)realloc(c->callbacks, sizeof(Callback *) * c->callback_size);
     Callback *f = (Callback *)malloc(sizeof(Callback));
     c->callbacks[c->callback_size-1] = f;
     f->pattern = strdup(pattern);
     f->func = strdup(func);
     f->type = type;
}

int register_source(lua_State *L) {
     addCallback(L, luaL_checkstring(L,1), lua_tostring(L,2), SOURCE);
     return 0;
}
int register_target(lua_State *L) {
     addCallback(L, luaL_checkstring(L,1), lua_tostring(L,2), TARGET);
     return 0;
}
int register_class(lua_State *L) {
     addCallback(L, luaL_checkstring(L,1), lua_tostring(L,2), CLASS);
     return 0;
}

static int msg_getvalue(struct tg_xap_msg msg[], int msg_len, 
			const char *section, const char *name, 
			char *value) 
{
     if(g_debuglevel) printf("msg_getvalue(%s,%s)\n", section, name);
     int i;
     for(i=0; i<msg_len; i++) {
	  if(strcasecmp(msg[i].section, section) == 0 &&
	     strcasecmp(msg[i].name, name) == 0)
	  {
	       strcpy(value, msg[i].value);
	       return 0;
	  }
     }
     return 1;
}

int lua_xapmsg_getvalue(lua_State *L) {
     const char *section = luaL_checkstring(L, 1);
     const char *name = luaL_checkstring(L, 2);
     if(section == NULL || name == NULL) return;

     char value[XAP_MAX_KEYVALUE_LEN];
     msg_getvalue(receivedMsg->msg, receivedMsg->elements, section, name, value);
     lua_pushstring(L, value);	  
     return 1; // one arg pushed to the stack.
}

// An LUA wrapper for the C call xap_send_message.  
// With the benefit that only a class and target need to be specified in the xap-header,
// all the other information will be setup/overriden by the normalization call.
int lua_xap_send_message(lua_State *L) {
     const char *msg = luaL_checkstring(L, 1);
     if(msg == NULL) {
	  if(g_debuglevel) printf("ERR: Cannot send a NULL message\n");
	  return;
     }
     char *nmsg = normalize_xap((char *)msg);
     if(nmsg) {
	  xap_send_message(nmsg);
	  free(nmsg);
     }
     return 0;  //no stack return values.
}

/*
  Find the CACHE entry for these fields.
  key = xapcache_find("source","target","class")
  xapcache_getvalue(key, "output.state","level")
 */
int lua_xapcache_find(lua_State *L) {
     const char *source = luaL_checkstring(L, 1);
     const char *target = luaL_checkstring(L, 2);
     const char *class = luaL_checkstring(L, 3);

     xapEntry *msg = findMessage(source, target, class);
     if(msg == NULL) return 0;

     lua_pushlightuserdata(L, msg);
     return 1;
}

int lua_xapcache_getvalue(lua_State *L) {
     xapEntry *entry = (xapEntry *)lua_touserdata(L, 1);
     const char *section = luaL_checkstring(L, 2);
     const char *name = luaL_checkstring(L, 3);
     if(entry == NULL || section == NULL || name == NULL) {
	  lua_pushstring(L, "?");
	  return 1;
     }

     char value[XAP_MAX_KEYVALUE_LEN];
     msg_getvalue(entry->msg, entry->elements, section, name, value);
     lua_pushstring(L, value);	  
     return 1;
}

int lua_xapcache_gettime(lua_State *L) {
     xapEntry *entry = (xapEntry *)lua_touserdata(L, 1);
     if(entry == NULL ) {
	  lua_pushinteger(L, 0);
	  return 1;
     }

     lua_pushinteger(L, entry->loaded);
     return 1;
}

Timer *findTimer(Xaplet *c, const char *name) {
     Timer **t = c->timers;
     int i;
     
     for(i=0; i<c->timer_size; i++) {
	  if(strcmp(name, t[i]->name)) continue;
	  return t[i];
     }
     return NULL;
}

int start_timer(lua_State *L) {
     const char *name = luaL_checkstring(L, 1);
     const int secs = luaL_checkint(L, 2);
     const char *func = luaL_checkstring(L, 3);
     
     Xaplet *c = findXapletFromState(L);
     if(c == NULL) return 0; // shouldn't happen.
     Timer *t = findTimer(c, name);
     if(t) { // Timer already exists?
	  // Allow a setup call to change the callback func.
	  free(t->func);
	  t->func = strdup(func);
     } else {
	  c->timer_size++;
	  c->timers = (Timer **)realloc(c->timers, sizeof(Timer *) * c->timer_size);
	  t = (Timer *)malloc(sizeof(Timer));
	  t->func = strdup(func);
	  t->name = strdup(name);
	  c->timers[0] = t;
     }
     t->time = time(NULL) + secs;

     if(g_debuglevel) printf("start_timer(%s,%d,%s)\n", t->name, t->time, t->func);

     return 0;
}

int stop_timer(lua_State *L) {
     const char *name = luaL_checkstring(L, 1);
     Xaplet *c = findXapletFromState(L);
     Timer *t = findTimer(c, name);
     if(t) {
	  t->time = -1; // Expire the timer.
     } // else we've been asked to delete a timer that doesn't exist?
}

/* C Functions that are available for the LUA scripts to call */
void register_our_funcs(lua_State *L) {
     lua_register(L, "register_source", register_source);
     lua_register(L, "register_target", register_target);
     lua_register(L, "register_class", register_class);
     lua_register(L, "xapmsg_getvalue", lua_xapmsg_getvalue);
     lua_register(L, "xap_send", lua_xap_send_message);
     lua_register(L, "xapcache_find", lua_xapcache_find);
     lua_register(L, "xapcache_getvalue", lua_xapcache_getvalue);
     lua_register(L, "xapcache_gettime", lua_xapcache_gettime);
     lua_register(L, "start_timer", start_timer);
     lua_register(L, "stop_timer", stop_timer);
}

/************ LUA script loading *****************/
extern int luaopen_rex_posix (lua_State *L);

void loadScript(char *file) {
     if(g_debuglevel) printf("loadScript(%s)\n", file);
     xaplet_size ++;
     xaplet = (Xaplet **)realloc(xaplet, sizeof(Xaplet *) * xaplet_size);
     Xaplet *c = (Xaplet *)malloc(sizeof(Xaplet));
     xaplet[xaplet_size-1] = c;

     c->callbacks = NULL;
     c->timers = NULL;
     c->timer_size = 0;
     c->callback_size = 0;
     c->state = luaL_newstate();
     c->filename = strdup(file);
     lua_gc(c->state, LUA_GCSTOP, 0);  /* stop collector during initialization */

     luaL_openlibs( c->state );
     register_our_funcs( c->state );

     // We can't call luaopen_rex_posix(c->state) directly, so push to stack and call.
     lua_pushcfunction(c->state, luaopen_rex_posix);
     lua_call(c->state, 0, 0);

     lua_gc(c->state, LUA_GCRESTART, 0);
     
     luaL_dofile( c->state, file );     

     // Call the scripts init() function
     lua_getglobal( c->state, "init");
     lua_call( c->state, 0, 0);
     // TODO: if this call fails remove the script.
}

// Search all files in the supplied directory and anything ending in .lua is loaded.
void loadScripts(char *path) {
     if(g_debuglevel) printf("loadScripts(%s)\n",path);

     // change our working dir.
     if(chdir(path)) {
	  perror(path); // Probably a bad path?
	  exit(1);
     }
     DIR *d = opendir(".");
     struct dirent *entry;
     while( (entry = readdir(d)) != NULL) {
	  char *p = entry->d_name + strlen(entry->d_name) - 4;
	  if(strcmp(p, ".lua") == 0)
	       loadScript(entry->d_name);
     }
}

void unloadScripts() {
     int i;
     for(i=0; i<xaplet_size; i++) {
	  lua_close(xaplet[i]->state);
	  free(xaplet[i]->filename);
	  free(xaplet[i]);
     }
     free(xaplet);
     xaplet = NULL;
     xaplet_size = 0;
}

void setfield(lua_State *L, char *index, char *value) {
     lua_pushstring(L, index);
     lua_pushstring(L, value);
     lua_settable(L, -3);
}

// XAP Message handler - Every XAP message will be processed here.
void xap_handler(const char *buf) {
     // Parse and add message to the cache.
     receivedMsg = addMessage(buf);
     if(receivedMsg == NULL) return;  // Not a message we care about (not cached).
     if(g_debuglevel > 7) dump_cacheEntry(receivedMsg);

     // check if any of the scripts are interested in this message.
     int i, j;
     for(i=0; i<xaplet_size; i++) { // for each script
	  
	  // As these values are accessed frequently in scripts
	  // we conviently expose them in a TABLE.
	  // xAPMsg.target, xAPMsg.source, xAPMsg.class
	  lua_State *L = xaplet[i]->state;
	  lua_getglobal(L, "xAPMsg");
	  if(!lua_istable(L, -1)) {
	       lua_newtable(L);
	  }
	  setfield(L, "target", receivedMsg->target);
	  setfield(L, "source", receivedMsg->source);
	  setfield(L, "class", receivedMsg->class);
	  lua_setglobal(L, "xAPMsg");

	  for(j=0; j<xaplet[i]->callback_size; j++) { // for each callback
	       Callback *cb = xaplet[i]->callbacks[j];
	       switch(cb->type) {
	       case TARGET: runLuaCallback(xaplet[i], cb, receivedMsg->target); break;
	       case SOURCE: runLuaCallback(xaplet[i], cb, receivedMsg->source); break;
	       case CLASS: runLuaCallback(xaplet[i], cb, receivedMsg->class); break;
	       }
	  }
     }
}

void fire_timer(lua_State *L, Timer *t) {
     if(g_debuglevel) printf("Fire Timer %s calling %s\n", t->name, t->func);
     lua_getglobal(L, t->func);
     lua_call( L, 0, 0 );
}

void check_timers() {
     int i, j;
     time_t now = time(NULL);

     for(j=0; j<xaplet_size; j++) {
	  Xaplet *c = xaplet[j];
	  for(i=0; i < c->timer_size; i++) {
	       Timer *t = c->timers[i];
	       if( t->time == -1 ) continue; // expired timer?
	       if( now > t->time ) {
		    fire_timer(c->state, t);
		    t->time = -1; // mark expired
	       }
	  }
     }
}

/* Where we spend 99% of our time waiting for a message 
*/
void process_loop() {
     char i_xap_buff[1500];
     struct timeval i_tv;
     fd_set i_rdfs, master;
     int ret;

     FD_ZERO(&master);
     FD_SET(g_xap_receiver_sockfd, &master);

     while(1) {
	  i_tv.tv_sec = 1;
	  i_tv.tv_usec = 0;
	  i_rdfs = master;

	  ret = select(g_xap_receiver_sockfd+1, &i_rdfs, NULL, NULL, &i_tv);
	  
	  if (FD_ISSET(g_xap_receiver_sockfd, &i_rdfs)) {
	       if(xap_poll_incoming(g_xap_receiver_sockfd, i_xap_buff, sizeof(i_xap_buff))>0) {
		    xap_handler(i_xap_buff);
	       }
	  }

	  check_timers();

	  // Send heartbeat periodically
	  xap_heartbeat_tick(HBEAT_INTERVAL);
     }
}


void setupXAPini() 
{
     char uid[5];
     char guid[9];
     long n;

     // default to 00D8 if not present
     n = ini_gets("plugboard","uid","00D8",uid,sizeof(uid),inifile);

     // validate that the UID can be read as HEX
     if(n == 0 || !(isxdigit(uid[0]) && isxdigit(uid[1]) && 
		    isxdigit(uid[2]) && isxdigit(uid[3]))) 
     {
	  strcpy(uid,"00D8");
     }
     snprintf(guid,sizeof guid,"FF%s00", uid);
     XAP_GUID = strdup(guid);

     char i_control[64];
     char s_control[128];
     n = ini_gets("xap","instance","",i_control,sizeof(i_control),inifile);
     strcpy(s_control, "livebox");
     // If there a unique HAH sub address component?
     if(i_control[0]) {
	     strlcat(s_control, ".",sizeof(s_control));
	     strlcat(s_control, i_control, sizeof(s_control));
     }
     XAP_SOURCE = strdup(s_control);

     ini_gets("plugboard","scriptdir","/etc/plugboard",scriptdir,sizeof(scriptdir),inifile);
}

int main( int argc, char *argv[] )
{
	printf("\nxAP Plugboard - for xAP v1.2\n");
	printf("Copyright (C) DBzoo 2009\n\n");

	setupXAPini();
	xap_init(argc, argv, 0);

	loadScripts(scriptdir);

	printf("Ready\n");
	process_loop();
}
