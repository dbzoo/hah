/* $Id$
   Copyright (c) Brett England, 2010

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.

   SWIG(2.0.1) xaplib2 MAPPING for LUA
*/
%module xap

%{
	#include "xap.h"
	#include "bsc.h"
	#include <stdlib.h>
	#include <string.h>

	typedef xAPFrame Frame;
	typedef unsigned int Boolean;
%}

/* the extra wrappers for lua functions,see SWIG/Lib/lua/lua_fnptr.i for more details */
%include "lua_fnptr.i"

%{
	typedef struct {
		xAPFilter *f;
		SWIGLUA_REF fn;
	} Filter;

	typedef struct {
		int interval;
		SWIGLUA_REF fn;
		xAPTimeoutCallback *cb;
	} Timer;

	typedef struct {
		SWIGLUA_REF fn;
		xAPSocketConnection *cb;
	} Select;

	typedef struct {
		bscEndpoint *list;
	} BSC;
%}

#define MSG_HBEAT  1		// special control message, type heartbeat
#define MSG_ORDINARY  2		// ordinary xap message type
#define MSG_CONFIG_REQUEST 3	// xap config message type
#define MSG_CACHE_REQUEST 4		// xap cache message type
#define MSG_CONFIG_REPLY 5		// xap config message type
#define MSG_CACHE_REPLY 6		// xap cache message type
#define MSG_UNKNOWN  0              // unknown xap message type
#define MSG_NONE 0                  // (or no message received)

// parse.c

// Create a Boolean type
%apply bool { Boolean };
%typemap(in) Boolean %{ $1 = $input ? 1 : 0; %}

/** The Frame contents cannot be accessed by LUA directly only via the class methods.
 * Rename xAPFrame as Frame so in LUA <module>.Frame can be used.
*/
typedef struct {
    %immutable;
	int len;
    %mutable;
    %extend {
	Frame(char *msg) {
		xAPFrame *f = (xAPFrame *)calloc(1,sizeof(xAPFrame));
		// As we do't' have an (LUA_State *) we can push an error back with
		// just create an empty object if NULL is supplied.
		if(msg != NULL) {
			f->len = strlcpy(f->dataPacket, msg, sizeof(f->dataPacket));
		        parseMsgF(f);
		}
		return f;
	}

	char *getValue(char *section, char *key) {
	  return xapGetValueF(self, section, key);
	}

	int getType() {
	  return xapGetTypeF(self);
	}

	Boolean isValue(char *section, char *key, char *value) {
	  return xapIsValueF(self,section,key,value);
	}

	// Should we rebuild or simply strdup() that supplied in the constructor and return it?
	const char *__tostring() {
		static char newmsg[XAP_DATA_LEN];
		parsedMsgToRawF(self, newmsg, sizeof(newmsg));
		return newmsg;
	}
    }
} Frame;

%rename(getType) xapGetType;
int xapGetType();
%rename(getValue) xapGetValue;
char *xapGetValue(char *section, char *key);
%rename(isvalue) xapIsValue;
Boolean xapIsValue(char *section, char *key, char *value);

// filter.c

%rename(FILTER_ANY) XAP_FILTER_ANY;
%rename(FILTER_ABSENT) XAP_FILTER_ABSENT;
const char *const XAP_FILTER_ANY;
const char *const XAP_FILTER_ABSENT;

%inline %{

void filterCB(void *user_data) {
	SWIGLUA_REF *fn = (SWIGLUA_REF *)user_data;
	swiglua_ref_get(fn);
	lua_call(fn->L,0,0); // 0 in, 0 out
}

%}

typedef struct {
	// We don't export any internals.
	%extend {
		void add(char *section, char *key, char *value) {
			xapAddFilter(&(self->f), section, key, value);
		}

		/** Export a string representation of the FILTER linked list
		 * This is a LUA table representation which might be handy later.
		 */
		const char *__tostring() {
			static char out[256];
			xAPFilter *f;
			int len = 1;
			strcpy(out,"{");
			LL_FOREACH(self->f, f) {
			   len += snprintf(&out[len],sizeof(out)-len,"{'%s','%s','%s'}",f->section,f->key,f->value);
			   if(f->next) {
				strlcat(out,",", sizeof(out)-len);
				len++;
			   }
			}
			strlcat(out,"}",sizeof(out));
			return out;
		}

		/** Callback a LUA function when the filters all match
		 * @param fn the location on the stack of the function
		*/
	        int callback(SWIGLUA_FN fn) {
			SWIGLUA_FN_GET(fn);
			swiglua_ref_set(&self->fn, fn.L, fn.idx);

			if(gXAP == NULL) {
				lua_pushstring(fn.L,"init() not called");
				SWIG_fail;
			}

			// Pass the stored FN reference as USER DATA.
			// An internal function will convert and make the LUA call.
			xapAddFilterAction(&filterCB, self->f, &self->fn);
			return 0;

			fail:
			   lua_error(fn.L);
			   return 1;
		}
 
	}
} Filter;

// timeout.c

%inline %{
	void timeoutCB(int interval, void *data) {
	  	Timer *t = (Timer *)data;
		swiglua_ref_get(&t->fn);
		// It is better to push back SELF.
		// then you'd be able to get interval anyway.
		//lua_pushnumber(t->fn.L, interval);
		SWIG_NewPointerObj(t->fn.L,t,SWIGTYPE_p_Timer,1);
		lua_call(t->fn.L,1,0); // 1 in, 0 out
	}
%}

%nodefaultdtor Timer;
typedef struct {
  int interval;
  %extend {
	Timer(SWIGLUA_FN fn, int interval) {
		SWIGLUA_FN_GET(fn);
		if(gXAP == NULL) {
			lua_pushstring(fn.L,"init() not called");
			SWIG_fail;
		}
		Timer *t = (Timer *)malloc(sizeof(Timer));
		t->interval = interval;
		t->cb = NULL;
		swiglua_ref_set(&t->fn, fn.L, fn.idx);
		return t;
	     fail:
		lua_error(fn.L);
		return NULL;
	}

	Timer *stop() {
		xapDelTimeoutAction(self->cb);
		self->cb = NULL;
		return self;
	}

	Timer *start() {
		if(self->cb == NULL) {
			self->cb = xapAddTimeoutAction(&timeoutCB, self->interval, self);
		}
		xapTimeoutReset(self->cb);
		return self;
	}

	Timer *reset() {
		if(self->cb) xapTimeoutReset(self->cb);
		return self;
	}

	void delete() {
		if(self->cb) Timer_stop(self);
		free(self);
	}
   }	
} Timer;

// init.c

%inline %{
	void selectCB(int fd, void *data) {
	  	Select *t = (Select *)data;
		swiglua_ref_get(&t->fn);
		lua_pushnumber(t->fn.L, fd);
		lua_call(t->fn.L,1,0); // 0 in, 0 out
	}
%}

%nodefaultdtor Select;
typedef struct {
  %extend {
	Select(SWIGLUA_FN fn, int fd) {
		SWIGLUA_FN_GET(fn);
		if(gXAP == NULL) {
			lua_pushstring(fn.L,"init() not called");
			SWIG_fail;
		}
		Select *s = (Select *)malloc(sizeof(Select));
		swiglua_ref_set(&s->fn, fn.L, fn.idx);
		s->cb = xapAddSocketListener(fd, &selectCB, s);
		return s;
	     fail:
		lua_error(fn.L);
		return NULL;
	}

	void delete() {
		xapDelSocketListener(self->cb);
		self->cb = NULL;
		free(self);
	}
   }	
} Select;

%rename(init) xapInit;
void xapInit(char *source, char *uid, char *interfaceName="br0");
%rename(getSource) xapGetSource;
char *xapGetSource();
%rename(getUID) xapGetUID;
char *xapGetUID();
%rename(getIP) xapGetIP;
char *xapGetIP();

// tx.c

%rename(send) xapSend;
void xapSend(const char *mess);
%rename(fillShort) fillShortXap;
char *fillShortXap(char *shortMsg);

void sendShort(const char *);
%{
	// A common happy meal
	void sendShort(const char *m) {
		xapSend(fillShortXap((char *)m));
	}
%}


// rx.c

%rename(process) xapProcess;
void xapProcess();

// logging
void setLoglevel(int);
int getLoglevel();

// bsc.c
#define BSC_BINARY 0
#define BSC_LEVEL 1
#define BSC_STREAM 2

#define BSC_INPUT 0
#define BSC_OUTPUT 1

#define BSC_STATE_OFF 0
#define BSC_STATE_ON 1
#define BSC_STATE_UNKNOWN 3

#define BSC_INFO_CLASS "xAPBSC.info"
#define BSC_EVENT_CLASS "xAPBSC.event"
#define BSC_CMD_CLASS "xAPBSC.cmd"
#define BSC_QUERY_CLASS "xAPBSC.query"

%{
	struct bscLuaCB {
		SWIGLUA_REF cmd;
		SWIGLUA_REF infoevent;
	};

	void bscLuaCmd(bscEndpoint *e) {
		// Pull the LUA callback from userData;
		struct bscLuaCB *data = (struct bscLuaCB*)(e->userData);
		swiglua_ref_get(&data->cmd);
		lua_State *L = data->cmd.L;

		// Push (bscEndpoint)
		SWIG_NewPointerObj(L, e, SWIGTYPE_p_bscEndpoint, 1);

		// Call user function
		lua_call(L,1,0); // 1 in, 0 out
	}

	int bscLuaInfoEvent(bscEndpoint *e, char *clazz) {
		// Pull the LUA callback from userData->infoevent
		struct bscLuaCB *data = (struct bscLuaCB*)(e->userData);
		swiglua_ref_get(&data->infoevent);
		lua_State *L = data->infoevent.L;

		// Push (bscEndpoint, class)
		SWIG_NewPointerObj(L, e, SWIGTYPE_p_bscEndpoint, 1);
		lua_pushstring(L, clazz);

		// Call user function
		lua_call(L,2,1); // 2 in, 1 out		

		// retrieve result
		// If function returns TRUE we propagate the event and really do it.
		if (!lua_isboolean(L, -1)) {
			 luaL_error(L, "function must return a boolean");
			 // longjmp - no return
		}
		int z = lua_toboolean(L, -1);
		lua_pop(L, 1);  /* pop returned value */
		return z;
	}
%}

// bscEndpoints cannot be directly created by LUA
// They are C structs managed as part of the BSC() class.
%nodefaultctor bscEndpoint;
%nodefaultdtor bscEndpoint;

typedef struct {
	%extend {
		void setState(int state) {
			bscSetState(self, state);
		}
		int getState() {
			return self->state;
		}
		void setText(char *text) {
			bscSetText(self, text);
		}
		char *getText() {
			return self->text;
		}
		void setLevel(char *text) {
			bscSetLevel(self, text);
		}
		char *getLevel() {
			return self->level;
		}
		void setDisplayText(char *text) {
			bscSetDisplayText(self, text);
		}
		char *getDisplayText() {
			return self->displayText;
		}

		char *getName() {
			static char addr[512];
			strlcpy(addr, self->name, sizeof addr);
			if(self->subaddr) {
				strlcat(addr, ".", sizeof addr);
				strlcat(addr, self->subaddr, sizeof addr);
			}
			return addr;
		}
		char *getSource() {
			return self->source;
		}
		char *__tostring() {
			return bscEndpoint_getName(self);
		}
	}
} bscEndpoint;


%nodefaultdtor BSC;

typedef struct {
	%extend {
		bscEndpoint *add(char *name, char *subaddr, int inout, int type, SWIGLUA_FN cmd, SWIGLUA_FN infoevent ) {
			bscEndpoint *e = bscAddEndpoint(&self->list, name, subaddr, inout, type, bscLuaCmd, bscLuaInfoEvent);

			// Store the LUA callbacks as Endpoint userdata
			struct bscLuaCB *luaUserData = (struct bscLuaCB *)calloc(1,sizeof(struct bscLuaCB));

			SWIGLUA_FN_GET(cmd);
			swiglua_ref_set(&luaUserData->cmd, cmd.L, cmd.idx);
			
			SWIGLUA_FN_GET(infoevent);
			swiglua_ref_set(&luaUserData->infoevent, infoevent.L, infoevent.idx);

			e->userData = luaUserData;
			bscAddEndpointFilter(e,120);
			return e;
		}

		bscEndpoint *findEndpoint(char *name, char *subaddr) {
			return bscFindEndpoint(self->list, name, subaddr);
		}

	}
} BSC;
