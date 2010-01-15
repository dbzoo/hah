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
static const char data[] = {
0xCB, 0xC9, 0xCC, 0xCB, 0x56, 0x48, 0xCD, 0x49, 0x4D, 0x4F, 0xCC, 0x2B, 
0x01, 0x00, };
static embfile_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_FILE;           
   e.res.filename = "/www/style/.svn/text-base/current.svn-base";        
   e.data = (unsigned char*)data;  
   e.size = sizeof(data);          
   e.file_size = 12;               
   e.mime_type = "application/octet-stream";           
   e.mtime = 1260557763;                  
   e.comp = 1;                    
   e.encrypted = 0;               
}                                  
#ifdef __cplusplus                 
extern "C" {                     
#endif                             
void module_init_7a5591d5887558c3eba3cb49f9b01f52(void);         
void module_init_7a5591d5887558c3eba3cb49f9b01f52(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_7a5591d5887558c3eba3cb49f9b01f52(void);         
void module_term_7a5591d5887558c3eba3cb49f9b01f52(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
