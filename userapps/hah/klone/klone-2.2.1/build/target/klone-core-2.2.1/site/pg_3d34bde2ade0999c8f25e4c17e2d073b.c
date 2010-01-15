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
0xF3, 0x56, 0x30, 0x34, 0xE4, 0x2A, 0x2E, 0xCB, 0xB3, 0x2A, 0x2E, 0x48, 
0x4D, 0xCE, 0x4C, 0xCC, 0xE1, 0x0A, 0x53, 0x30, 0xE4, 0xD2, 0xE2, 0x72, 
0xF5, 0x73, 0xE1, 0x02, 0x00, };
static embfile_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_FILE;           
   e.res.filename = "/www/style/.svn/prop-base/current.svn-base";        
   e.data = (unsigned char*)data;  
   e.size = sizeof(data);          
   e.file_size = 27;               
   e.mime_type = "application/octet-stream";           
   e.mtime = 1260557763;                  
   e.comp = 1;                    
   e.encrypted = 0;               
}                                  
#ifdef __cplusplus                 
extern "C" {                     
#endif                             
void module_init_3d34bde2ade0999c8f25e4c17e2d073b(void);         
void module_init_3d34bde2ade0999c8f25e4c17e2d073b(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_3d34bde2ade0999c8f25e4c17e2d073b(void);         
void module_term_3d34bde2ade0999c8f25e4c17e2d073b(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
