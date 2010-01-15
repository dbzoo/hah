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
static const char *SCRIPT_NAME = "heading.kl1";
static int pageheading_kl1 (void) { static volatile int dummy; return dummy; }
static request_t *request = NULL;
static response_t *response = NULL;
static session_t *session = NULL;
static io_t *in = NULL;
static io_t *out = NULL;
static const char klone_html_zblock_0[] = {
0xB3, 0x49, 0xC9, 0x2C, 0x53, 0xC8, 0x4C, 0xB1, 0x55, 0xCA, 0xC9, 0x4F, 
0xCF, 0x57, 0xB2, 0xB3, 0xC9, 0x30, 0xB4, 0x73, 0x2C, 0x2D, 0xC9, 0xCF, 
0x4D, 0x2C, 0xC9, 0xCC, 0xCF, 0x53, 0xF0, 0x28, 0x4D, 0xB2, 0xD1, 0x07, 
0x0A, 0xD9, 0xE8, 0x03, 0x95, 0xD9, 0x71, 0x01, 0x00, };


static void exec_page(dypage_args_t *args)
{
   request = args->rq;                       
   response = args->rs;                      
   session = args->ss;                      
   in = request_io(request);           
   out = response_io(response);        
   u_unused_args(SCRIPT_NAME, request, response, session, in, out); 
   pageheading_kl1 () ; 
 

dbg_if(u_io_unzip_copy(out, klone_html_zblock_0,    sizeof(klone_html_zblock_0)));
goto klone_script_exit;
klone_script_exit:     
   return;             
}                      
static embpage_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_PAGE;           
   e.res.filename = "/www/component/heading.kl1";        
   e.fun = exec_page;              
}                                  
#ifdef __cplusplus                 
extern "C" {                     
#endif                             
void module_init_e35c8a1874f880a411215b1dafdc5363(void);         
void module_init_e35c8a1874f880a411215b1dafdc5363(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_e35c8a1874f880a411215b1dafdc5363(void);         
void module_term_e35c8a1874f880a411215b1dafdc5363(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
