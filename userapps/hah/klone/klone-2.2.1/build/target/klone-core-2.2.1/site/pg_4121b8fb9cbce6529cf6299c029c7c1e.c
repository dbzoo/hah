/*
 * Copyright (c) 2005, 2006, 2006 by KoanLogic s.r.l. <http://www.koanlogic.com>
 * All rights reserved.
 *
 * This file is part of KLone, and as such it is subject to the license
 * stated in the LICENSE file which you have received as part of this
 * distribution
 *
 */
#include <klone/emb.h>
#include <klone/dypage.h>
static const char *SCRIPT_NAME = "login.kl1";
static int pagelogin_kl1 (void) { static volatile int dummy; return dummy; }
static request_t *request = NULL;
static response_t *response = NULL;
static session_t *session = NULL;
static io_t *in = NULL;
static io_t *out = NULL;
 #line 1 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/login.kl1" 

 /* -*- mode:c;coding:utf-8; -*- */
#include "minIni.h"
#include "const.h"

/*  
 * $Id: login.kl1,v 1.7 2009/05/22 21:29:36 brett Exp brett $
 * Login page.
 */

// Carve up a string delimited by a comma
// ex:   hello,world,foobar
// arg[0] = hello
// arg[1] = world
// arg[2] = foobar
// return 3
int commaTok(char *arg[], int arglen, char *str)
{
	 char *p = str;
	 int j=0;
	 arg[j++] = str;
	 do {
		  if(*p == ',') {
			   arg[j++] = p+1;
			   *p = 0;
			   if (j == arglen) return j;
		  }
		  p++;
	 } while (*p);
	 return j;
}


// Check if the CURRENT page is allowed.
int isPageAllowed(session_t *ss, char *page) {
	 char *username = session_get(ss, "username");

	 // Hmm no session username guess we will allow access (by default).
	 if (username == NULL || *username == 0) return 1;

	 // The admin user has no page restrictions
	 if(strcmp("admin", username) == 0) return 1;

	// The menu system will hide the pages from access
	// However we still need to handle DIRECT url navigation.
	char ini_pages[128];
	int n = ini_gets("security", username, "", ini_pages, sizeof(ini_pages), con.inifile);
	if (n == 0) return 1; // No defined pages? (allow all access)
	char *pages[20];
	int a = commaTok(pages, sizeof(pages), ini_pages);

	// Is the current page allowed for this user?
	while(a--) {
		 if(strcmp(pages[a], page) == 0) return 1;
	}
	return 0;
}

// Check if the script/page is allowed to be accessed.
int isScriptAllowed(session_t *ss, response_t *rp, char *script) {
	 // Always allow access to this
	 if (strcmp("index.kl1", script) == 0) return 1;

	// Strip .kl1 from the script
	char page[20];
	strncpy(page, script, sizeof(page));
	char *p = strchr(page, '.');
	if (p) *p = 0;

	return isPageAllowed(ss, page);
}

void menu_item(FILE *out, session_t *ss, response_t *rp, char *script, char *label) {
	 if(isScriptAllowed(ss, rp, script)) {
		  io_printf(out,"<li><a href=\"%s\">%s</a></li>", script, label);
	 }
}

/*
 * If user not logged, force login.
 */
int login_force(session_t *ss, request_t *rq, response_t *rp, const char *script) 
{
	char *username;
	
	username = session_get(ss, "username");
	if (!username || strlen(username) == 0) {
		session_set(ss, "after_login", script);
		response_redirect(rp, "login.kl1");
		return 0;
	}

	// Normally the menu system wouldn't show the page, but check for direct access.
	// Don't allow access (redirect)
	if(isScriptAllowed(ss, rp, script)) return 1;

	response_redirect(rp, "index.kl1");
	return 0;
} /* force_login() */

int isValidUser(char *user, char *passwd) {
	 long n;
	 char md5[MD5_DIGEST_BUFSZ];
	 // compute MD5 checksum of entered password
	 u_md5(passwd, strlen(passwd), md5);

	 if(user == NULL || passwd == NULL) return 0;

	 // Admin is special it has its own INI entry.
	 if(strcmp("admin",user) == 0) {
		  char ini_passwd[MD5_DIGEST_BUFSZ];
		  
		  n = ini_gets("security","admin_passwd", "", ini_passwd, sizeof(ini_passwd), con.inifile);
		  if(n == 0) {   // No password found?
			   return 1; // Accept whatever was entered.
		  }
		  return strcmp(ini_passwd, md5) == 0;
	 }

	 // Try one of the NAMED users
	 char ini_users[128];
	 char ini_passwd[10*MD5_DIGEST_BUFSZ];
	 char *users[10];
	 char *passwds[10];
	 
	 n = ini_gets("security","user", "", ini_users, sizeof(ini_users), con.inifile);
	 if (n == 0) return 0;  // No user ini entry
	 n = ini_gets("security","passwd", "", ini_passwd, sizeof(ini_passwd), con.inifile);
	 if (n == 0) return 0;  // No passwd ini entry

	 int a = commaTok(users, sizeof(users), ini_users);
	 int b = commaTok(passwds, sizeof(passwds), ini_passwd);
	 if(a != b) { // What the hell?  The arrays are different sizes!
		  return 0;  // I'm outta here..
	 }

	 while(a--) {
		  // Compare ini_passwd to the clear text version (for now).
		  if(strcmp(users[a], user) == 0 && strcmp(passwds[a], passwd) == 0) {
			   return 1;
		  }
	 }
	 return 0;
}

#line 166 "pg_4121b8fb9cbce6529cf6299c029c7c1e.c"
static const char klone_html_zblock_0[] = {
0x65, 0x52, 0xC1, 0x4E, 0xE3, 0x30, 0x10, 0xBD, 0xF7, 0x2B, 0x2C, 0x9F, 
0x40, 0xAB, 0xC6, 0x5B, 0xA4, 0x95, 0x60, 0x71, 0x22, 0xA1, 0xBD, 0x70, 
0x80, 0x13, 0xDC, 0x56, 0x7B, 0x70, 0x9C, 0x69, 0xED, 0xC5, 0x89, 0x83, 
0x3D, 0x2E, 0xED, 0xDF, 0x33, 0x8E, 0x43, 0x69, 0xE0, 0x64, 0xFB, 0xCD, 
0x9B, 0xF7, 0x66, 0x5E, 0xB2, 0x92, 0x06, 0x7B, 0xD7, 0xAC, 0xA4, 0x01, 
0xD5, 0xD1, 0xD1, 0x03, 0x2A, 0x66, 0x10, 0xC7, 0x35, 0xBC, 0x26, 0xBB, 
0xAF, 0xF9, 0x1F, 0x3F, 0x20, 0x0C, 0xB8, 0x7E, 0x3E, 0x8E, 0xC0, 0x99, 
0x2E, 0xAF, 0x9A, 0xAB, 0x71, 0x74, 0x56, 0x2B, 0xB4, 0x7E, 0x10, 0x87, 
0x2C, 0xF1, 0xE3, 0xD0, 0xBB, 0x5B, 0xA6, 0x8D, 0x0A, 0x11, 0xB0, 0xB6, 
0xD1, 0xAF, 0xAF, 0xAF, 0x7F, 0xDD, 0xAC, 0x37, 0x9C, 0x09, 0x92, 0x75, 
0x76, 0x78, 0x61, 0x48, 0x12, 0x35, 0x47, 0x38, 0xA0, 0xD0, 0x31, 0x72, 
0x16, 0xC0, 0xD5, 0x3C, 0xE2, 0xD1, 0x41, 0x34, 0x00, 0xC8, 0x99, 0x09, 
0xB0, 0x9D, 0x11, 0xA1, 0x53, 0x08, 0xE4, 0x24, 0xA6, 0x57, 0x35, 0xF1, 
0xB3, 0x10, 0x5A, 0x74, 0xD0, 0x3C, 0xD8, 0x3D, 0xB4, 0xFE, 0x20, 0x45, 
0x79, 0xAE, 0xA4, 0x98, 0xC7, 0x6F, 0x7D, 0x77, 0x64, 0xDA, 0xA9, 0x18, 
0x6B, 0xBE, 0x4B, 0x96, 0x33, 0x3F, 0x38, 0xAF, 0xBA, 0x9A, 0xFF, 0x57, 
0x7B, 0x15, 0x75, 0xB0, 0x23, 0xFE, 0xEE, 0xBC, 0x4E, 0x3D, 0x49, 0x57, 
0x5B, 0x1F, 0xFA, 0xF8, 0xF7, 0xE7, 0xBF, 0x2A, 0x45, 0x08, 0x83, 0xEA, 
0x81, 0x10, 0x9D, 0xE2, 0xC5, 0xE5, 0x2D, 0x27, 0xA9, 0xCE, 0xEE, 0x99, 
0xA5, 0xCE, 0xB7, 0x40, 0xBB, 0x42, 0x38, 0x87, 0xE6, 0x14, 0xCE, 0xA1, 
0xEC, 0x3F, 0x91, 0x3E, 0x21, 0xE7, 0x77, 0x9E, 0x37, 0xD2, 0x6C, 0x9A, 
0xBB, 0x84, 0xBE, 0x9F, 0xB2, 0x62, 0xF7, 0xA9, 0xA5, 0x61, 0x37, 0x8D, 
0x14, 0x44, 0x23, 0xF6, 0x27, 0x9D, 0x46, 0x4A, 0xD4, 0xCF, 0x16, 0xC0, 
0xA3, 0xB2, 0x43, 0x06, 0x99, 0x4C, 0x6E, 0x01, 0x3E, 0xD8, 0x88, 0x53, 
0x81, 0x51, 0xB6, 0x8D, 0x54, 0x73, 0x76, 0xEA, 0xE4, 0x54, 0xBD, 0xB8, 
0x0D, 0x3F, 0x73, 0x96, 0x42, 0x91, 0x29, 0x71, 0xBF, 0xF5, 0xD0, 0xF7, 
0x4A, 0x63, 0xA1, 0x3F, 0xE5, 0xEB, 0x82, 0xB9, 0x1C, 0xE7, 0x29, 0xB5, 
0xC5, 0x74, 0x39, 0x10, 0xC1, 0x65, 0x1E, 0x29, 0x92, 0x9B, 0xDB, 0xCA, 
0x7E, 0xE5, 0x5A, 0xB4, 0xE6, 0xE2, 0x5C, 0xF9, 0x08, 0xE0, 0x74, 0x9E, 
0x7C, 0xCA, 0xCA, 0x5F, 0xC3, 0x8E, 0x19, 0x6B, 0x43, 0xFE, 0x07, 0xCC, 
0x55, 0xDE, 0xCB, 0x10, 0x98, 0xFF, 0x40, 0xA0, 0x3C, 0xAF, 0x8A, 0xD3, 
0xB3, 0xB1, 0x91, 0x8D, 0x6A, 0x07, 0x2C, 0x9F, 0xC1, 0x23, 0x68, 0x84, 
0xAE, 0x9A, 0x6A, 0xEF, };
static const char klone_html_zblock_1[] = {
0xE3, 0xB2, 0x49, 0xCB, 0x2F, 0xCA, 0x55, 0x48, 0x4C, 0x2E, 0xC9, 0xCC, 
0xCF, 0xB3, 0x55, 0x02, 0x00, };
static const char klone_html_zblock_2[] = {
0x95, 0x51, 0xCB, 0x6E, 0xC3, 0x20, 0x10, 0x3C, 0x37, 0x5F, 0x81, 0xF6, 
0x07, 0x50, 0xAF, 0x15, 0xF6, 0xB1, 0x97, 0xF6, 0x27, 0x70, 0xD8, 0x38, 
0x2B, 0xAD, 0xC1, 0x82, 0x75, 0xDA, 0xFE, 0x7D, 0x17, 0xBF, 0x12, 0x2B, 
0x52, 0xA5, 0x72, 0x1B, 0x66, 0x86, 0x19, 0x0D, 0x60, 0x06, 0x94, 0x6B, 
0x0A, 0x0D, 0x8C, 0xA9, 0x08, 0xB4, 0x27, 0x63, 0xDC, 0x85, 0x90, 0x43, 
0x41, 0x99, 0x01, 0x63, 0x8F, 0x31, 0xB4, 0x9F, 0xA9, 0xA7, 0xE8, 0xEC, 
0x8A, 0x2A, 0x11, 0xE8, 0x66, 0xCE, 0xEC, 0x4B, 0x69, 0x40, 0xB5, 0x42, 
0xB1, 0x57, 0xF7, 0xCB, 0xE1, 0x9E, 0x7D, 0x87, 0x0C, 0xED, 0x54, 0x30, 
0x47, 0x3F, 0xE0, 0x9B, 0xB3, 0x4A, 0x56, 0x73, 0x3D, 0x8E, 0xE2, 0x38, 
0x89, 0x91, 0x9F, 0x11, 0x1B, 0x10, 0xFC, 0x16, 0x30, 0x55, 0xD4, 0xC0, 
0x26, 0x07, 0x3B, 0x4B, 0x57, 0xD3, 0xFF, 0x22, 0x47, 0x05, 0x5F, 0x29, 
0x87, 0x3F, 0x22, 0x37, 0xC9, 0x16, 0xBB, 0xE3, 0x43, 0xAC, 0xB3, 0xF7, 
0x35, 0x5C, 0x97, 0x95, 0x3B, 0xB4, 0x98, 0xBA, 0x81, 0xE4, 0x3D, 0x25, 
0xC1, 0xBC, 0x6C, 0xF7, 0x98, 0xB0, 0xB0, 0x60, 0x6E, 0x9E, 0x27, 0x85, 
0x5C, 0x27, 0xD4, 0xD7, 0x9D, 0x95, 0xF0, 0xA4, 0xBD, 0x52, 0x08, 0x18, 
0x77, 0xED, 0xEB, 0xD6, 0xCA, 0x9F, 0x85, 0x52, 0x75, 0x9D, 0xEE, 0x85, 
0x52, 0x1E, 0x74, 0x8F, 0x1D, 0x2F, 0xF3, 0xCC, 0xB5, 0x48, 0xFF, 0xF1, 
0xC2, 0xC9, 0xCB, 0x07, 0x31, 0xD7, 0x4A, 0x2B, 0xFB, 0x0B, };
 #line 1 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/component/statusinfo.kl1" 


#include "const.h"
#include <time.h>
#include <sys/sysinfo.h>

#define FSHIFT          16              /* nr of bits of precision */
#define FIXED_1         (1<<FSHIFT)     /* 1.0 as fixed-point */
#define LOAD_INT(x) ((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1-1)) * 100)

#line 235 "pg_4121b8fb9cbce6529cf6299c029c7c1e.c"
static const char klone_html_zblock_3[] = {
0xE3, 0x02, 0x00, };
static const char klone_html_zblock_4[] = {
0xE3, 0xB2, 0x49, 0xC9, 0x2C, 0x53, 0xC8, 0x4C, 0xB1, 0x55, 0x2A, 0x2E, 
0x49, 0x2C, 0x29, 0x2D, 0xF6, 0xCC, 0x4B, 0xCB, 0x57, 0xB2, 0xE3, 0x02, 
0x8B, 0x26, 0xE7, 0x24, 0x16, 0x17, 0xDB, 0x2A, 0x65, 0x82, 0x85, 0x9C, 
0x4A, 0x33, 0x73, 0x52, 0xAC, 0x14, 0x00, };
static const char klone_html_zblock_5[] = {
0xB3, 0xD1, 0x4F, 0xC9, 0x2C, 0xB3, 0xE3, 0xB2, 0x01, 0x92, 0x0A, 0xC9, 
0x39, 0x89, 0xC5, 0xC5, 0xB6, 0x4A, 0x99, 0x79, 0x69, 0xF9, 0x4A, 0x76, 
0x00, };
static const char klone_html_zblock_6[] = {
0xB3, 0xD1, 0x4F, 0xC9, 0x2C, 0xB3, 0xE3, 0xB2, 0x01, 0x92, 0x0A, 0x99, 
0x29, 0xB6, 0x4A, 0x99, 0x79, 0x69, 0xF9, 0x4A, 0x76, 0xA5, 0x05, 0x5C, 
0x00, };
static const char klone_html_zblock_7[] = {
0xE3, 0xB2, 0xD1, 0x4F, 0xC9, 0x2C, 0xB3, 0xE3, 0x82, 0x52, 0x5C, 0x5C, 
0xA8, 0x7C, 0x1B, 0xFD, 0xA4, 0xFC, 0x94, 0x4A, 0x10, 0x9D, 0x51, 0x92, 
0x9B, 0x03, 0x94, 0x07, 0x00, };


static void exec_page(dypage_args_t *args)
{
   request = args->rq;                       
   response = args->rs;                      
   session = args->ss;                      
   in = request_io(request);           
   out = response_io(response);        
   u_unused_args(SCRIPT_NAME, request, response, session, in, out); 
   pagelogin_kl1 () ; 
 
 #line 1 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/component/heading.kl1" 
 
 #line 169 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/login.kl1" 
 
 #line 1 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/component/statusinfo.kl1" 
 
 #line 232 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/login.kl1" 
 
 #line 144 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/login.kl1" 



struct {
	char *username;
	char *password;
	char *destination;
} login;

login.username = request_get_arg(request, "username");
login.destination = session_get(session, "after_login");

if (!login.destination) login.destination = "index.kl1";


#line 291 "pg_4121b8fb9cbce6529cf6299c029c7c1e.c"

dbg_if(u_io_unzip_copy(out, klone_html_zblock_0,    sizeof(klone_html_zblock_0)));

 #line 191 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/login.kl1" 


switch (vars_get_value_i(request_get_args(request), "action"))
{
case 1:
    login.username = request_get_arg(request, "username");
    login.password = request_get_arg(request, "password");

     // authenticate here.
    if (isValidUser(login.username, login.password)) {
		 session_set(session, "username", login.username);
		 response_redirect(response, login.destination);
		 session_del(session, "after_login");
	} else {
		io_printf(out, "<p><b>Login failed, try again ...</b></p>");	
	}
}

#line 314 "pg_4121b8fb9cbce6529cf6299c029c7c1e.c"

dbg_if(u_io_unzip_copy(out, klone_html_zblock_1,    sizeof(klone_html_zblock_1)));

 #line 208 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/login.kl1" 

io_printf(out, "%s",
 SCRIPT_NAME 
);

#line 324 "pg_4121b8fb9cbce6529cf6299c029c7c1e.c"

dbg_if(u_io_unzip_copy(out, klone_html_zblock_2,    sizeof(klone_html_zblock_2)));


dbg_if(u_io_unzip_copy(out, klone_html_zblock_3,    sizeof(klone_html_zblock_3)));

 #line 11 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/component/statusinfo.kl1" 


time_t now = time(0);

#line 336 "pg_4121b8fb9cbce6529cf6299c029c7c1e.c"

dbg_if(u_io_unzip_copy(out, klone_html_zblock_4,    sizeof(klone_html_zblock_4)));

 #line 15 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/component/statusinfo.kl1" 

io_printf(out, "%s",
 con.build 
);

#line 346 "pg_4121b8fb9cbce6529cf6299c029c7c1e.c"

dbg_if(u_io_unzip_copy(out, klone_html_zblock_5,    sizeof(klone_html_zblock_5)));

 #line 16 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/component/statusinfo.kl1" 

io_printf(out, "%s",
 ctime(&now) 
);

#line 356 "pg_4121b8fb9cbce6529cf6299c029c7c1e.c"

dbg_if(u_io_unzip_copy(out, klone_html_zblock_6,    sizeof(klone_html_zblock_6)));

 #line 18 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/component/statusinfo.kl1" 


struct sysinfo info;
int updays, uphours, upminutes;

sysinfo(&info);

updays = (int) info.uptime / (60*60*24);
if (updays)
	 io_printf(out, " %d day%s, ", updays, (updays != 1) ? "s" : "");
upminutes = (int) info.uptime / 60;
uphours = (upminutes / 60) % 24;
upminutes %= 60;
if(uphours)
	 io_printf(out, "%2d:%02d, ", uphours, upminutes);
else
	 io_printf(out, "%d min, ", upminutes);

io_printf(out,"load average: %ld.%02ld, %ld.%02ld, %ld.%02ld\n", 
	   LOAD_INT(info.loads[0]), LOAD_FRAC(info.loads[0]), 
	   LOAD_INT(info.loads[1]), LOAD_FRAC(info.loads[1]), 
	   LOAD_INT(info.loads[2]), LOAD_FRAC(info.loads[2]));

#line 384 "pg_4121b8fb9cbce6529cf6299c029c7c1e.c"

dbg_if(u_io_unzip_copy(out, klone_html_zblock_7,    sizeof(klone_html_zblock_7)));
goto klone_script_exit;
klone_script_exit:     
   return;             
}                      
static embpage_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_PAGE;           
   e.res.filename = "/www/login.kl1";        
   e.fun = exec_page;              
}                                  
#ifdef __cplusplus                 
extern "C" {                     
#endif                             
void module_init_4121b8fb9cbce6529cf6299c029c7c1e(void);         
void module_init_4121b8fb9cbce6529cf6299c029c7c1e(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_4121b8fb9cbce6529cf6299c029c7c1e(void);         
void module_term_4121b8fb9cbce6529cf6299c029c7c1e(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
