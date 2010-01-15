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
0x55, 0x8D, 0x41, 0x0A, 0x83, 0x30, 0x10, 0x00, 0xEF, 0x79, 0x45, 0x3E, 
0x90, 0x10, 0xAA, 0x2D, 0x35, 0xDE, 0xFB, 0x8F, 0x48, 0xD6, 0xB0, 0x90, 
0xCD, 0x06, 0x5D, 0x41, 0x10, 0xFF, 0xAE, 0x52, 0x84, 0xF6, 0x3A, 0x33, 
0x30, 0x76, 0x5E, 0x06, 0x42, 0xF9, 0x30, 0x0B, 0x4C, 0x1A, 0x4B, 0x5D, 
0x44, 0x6F, 0xAA, 0x86, 0x18, 0xB1, 0x24, 0x6F, 0x9B, 0xD7, 0x03, 0x48, 
0xDB, 0xF6, 0xD9, 0x00, 0xF5, 0x6A, 0x57, 0x23, 0x42, 0x8E, 0x33, 0xFC, 
0x34, 0x46, 0xB8, 0x7A, 0xF7, 0xE7, 0x32, 0x24, 0x28, 0xF1, 0x4C, 0x28, 
0x4C, 0x09, 0x8B, 0xC9, 0x30, 0x8A, 0x37, 0x5D, 0x5D, 0xFB, 0x9B, 0x0C, 
0x2C, 0xC2, 0xE4, 0xDF, 0x17, 0xBA, 0x5F, 0x4E, 0x5B, 0xD7, 0x7D, 0x2F, 
0x07, };
static embfile_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_FILE;           
   e.res.filename = "/www/style/elegant/style_ie.css";        
   e.data = (unsigned char*)data;  
   e.size = sizeof(data);          
   e.file_size = 149;               
   e.mime_type = "text/css";           
   e.mtime = 1260557763;                  
   e.comp = 1;                    
   e.encrypted = 0;               
}                                  
#ifdef __cplusplus                 
extern "C" {                     
#endif                             
void module_init_8dfc08f346b96563e4e39b6422a2c6ff(void);         
void module_init_8dfc08f346b96563e4e39b6422a2c6ff(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_8dfc08f346b96563e4e39b6422a2c6ff(void);         
void module_term_8dfc08f346b96563e4e39b6422a2c6ff(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
