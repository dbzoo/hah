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
0xF3, 0x56, 0x30, 0x34, 0xE6, 0x2A, 0x2E, 0xCB, 0xB3, 0xCA, 0xCD, 0xCC, 
0x4D, 0xD5, 0x2D, 0xA9, 0x2C, 0x48, 0xE5, 0x0A, 0x53, 0x30, 0x32, 0xE1, 
0x4A, 0x2C, 0x28, 0xC8, 0xC9, 0x4C, 0x4E, 0x2C, 0xC9, 0xCC, 0xCF, 0xD3, 
0xCF, 0x4F, 0x2E, 0x49, 0x2D, 0xD1, 0x2D, 0x2E, 0x29, 0x4A, 0x4D, 0xCC, 
0xE5, 0x72, 0xF5, 0x73, 0xE1, 0x02, 0x00, };
static embfile_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_FILE;           
   e.res.filename = "/www/images/.svn/prop-base/throbber.gif.svn-base";        
   e.data = (unsigned char*)data;  
   e.size = sizeof(data);          
   e.file_size = 53;               
   e.mime_type = "application/octet-stream";           
   e.mtime = 1260557763;                  
   e.comp = 1;                    
   e.encrypted = 0;               
}                                  
#ifdef __cplusplus                 
extern "C" {                     
#endif                             
void module_init_c0b5fc44527e58e1f69c6c5c0f880b92(void);         
void module_init_c0b5fc44527e58e1f69c6c5c0f880b92(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_c0b5fc44527e58e1f69c6c5c0f880b92(void);         
void module_term_c0b5fc44527e58e1f69c6c5c0f880b92(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
