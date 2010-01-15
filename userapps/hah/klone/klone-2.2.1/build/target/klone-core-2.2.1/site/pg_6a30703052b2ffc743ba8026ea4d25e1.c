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
static const char *SCRIPT_NAME = "statusinfo.kl1";
static int pagestatusinfo_kl1 (void) { static volatile int dummy; return dummy; }
static request_t *request = NULL;
static response_t *response = NULL;
static session_t *session = NULL;
static io_t *in = NULL;
static io_t *out = NULL;
 #line 1 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/component/statusinfo.kl1" 


#include "const.h"
#include <time.h>
#include <sys/sysinfo.h>

#define FSHIFT          16              /* nr of bits of precision */
#define FIXED_1         (1<<FSHIFT)     /* 1.0 as fixed-point */
#define LOAD_INT(x) ((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1-1)) * 100)

#line 32 "pg_6a30703052b2ffc743ba8026ea4d25e1.c"
static const char klone_html_zblock_0[] = {
0xE3, 0x02, 0x00, };
static const char klone_html_zblock_1[] = {
0xE3, 0xB2, 0x49, 0xC9, 0x2C, 0x53, 0xC8, 0x4C, 0xB1, 0x55, 0x2A, 0x2E, 
0x49, 0x2C, 0x29, 0x2D, 0xF6, 0xCC, 0x4B, 0xCB, 0x57, 0xB2, 0xE3, 0x02, 
0x8B, 0x26, 0xE7, 0x24, 0x16, 0x17, 0xDB, 0x2A, 0x65, 0x82, 0x85, 0x9C, 
0x4A, 0x33, 0x73, 0x52, 0xAC, 0x14, 0x00, };
static const char klone_html_zblock_2[] = {
0xB3, 0xD1, 0x4F, 0xC9, 0x2C, 0xB3, 0xE3, 0xB2, 0x01, 0x92, 0x0A, 0xC9, 
0x39, 0x89, 0xC5, 0xC5, 0xB6, 0x4A, 0x99, 0x79, 0x69, 0xF9, 0x4A, 0x76, 
0x00, };
static const char klone_html_zblock_3[] = {
0xB3, 0xD1, 0x4F, 0xC9, 0x2C, 0xB3, 0xE3, 0xB2, 0x01, 0x92, 0x0A, 0x99, 
0x29, 0xB6, 0x4A, 0x99, 0x79, 0x69, 0xF9, 0x4A, 0x76, 0xA5, 0x05, 0x5C, 
0x00, };
static const char klone_html_zblock_4[] = {
0xE3, 0xB2, 0xD1, 0x4F, 0xC9, 0x2C, 0xB3, 0xE3, 0x82, 0x52, 0x00, };


static void exec_page(dypage_args_t *args)
{
   request = args->rq;                       
   response = args->rs;                      
   session = args->ss;                      
   in = request_io(request);           
   out = response_io(response);        
   u_unused_args(SCRIPT_NAME, request, response, session, in, out); 
   pagestatusinfo_kl1 () ; 
 

dbg_if(u_io_unzip_copy(out, klone_html_zblock_0,    sizeof(klone_html_zblock_0)));

 #line 11 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/component/statusinfo.kl1" 


time_t now = time(0);

#line 70 "pg_6a30703052b2ffc743ba8026ea4d25e1.c"

dbg_if(u_io_unzip_copy(out, klone_html_zblock_1,    sizeof(klone_html_zblock_1)));

 #line 15 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/component/statusinfo.kl1" 

io_printf(out, "%s",
 con.build 
);

#line 80 "pg_6a30703052b2ffc743ba8026ea4d25e1.c"

dbg_if(u_io_unzip_copy(out, klone_html_zblock_2,    sizeof(klone_html_zblock_2)));

 #line 16 "/home/brett/livebox/trunk/userapps/hah/klone/webapp/www/component/statusinfo.kl1" 

io_printf(out, "%s",
 ctime(&now) 
);

#line 90 "pg_6a30703052b2ffc743ba8026ea4d25e1.c"

dbg_if(u_io_unzip_copy(out, klone_html_zblock_3,    sizeof(klone_html_zblock_3)));

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

#line 118 "pg_6a30703052b2ffc743ba8026ea4d25e1.c"

dbg_if(u_io_unzip_copy(out, klone_html_zblock_4,    sizeof(klone_html_zblock_4)));
goto klone_script_exit;
klone_script_exit:     
   return;             
}                      
static embpage_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_PAGE;           
   e.res.filename = "/www/component/statusinfo.kl1";        
   e.fun = exec_page;              
}                                  
#ifdef __cplusplus                 
extern "C" {                     
#endif                             
void module_init_6a30703052b2ffc743ba8026ea4d25e1(void);         
void module_init_6a30703052b2ffc743ba8026ea4d25e1(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_6a30703052b2ffc743ba8026ea4d25e1(void);         
void module_term_6a30703052b2ffc743ba8026ea4d25e1(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
