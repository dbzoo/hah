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
0x15, 0x8A, 0xBD, 0x0E, 0x40, 0x30, 0x14, 0x46, 0xF7, 0xFB, 0x14, 0x3C, 
0x80, 0x7C, 0x89, 0xA8, 0xA1, 0x33, 0x93, 0xC4, 0x68, 0x95, 0xE2, 0x26, 
0x44, 0xD3, 0xCA, 0xAD, 0x96, 0xC7, 0x57, 0xE3, 0xF9, 0x19, 0x8A, 0x5A, 
0x51, 0x48, 0x4E, 0x3F, 0xAB, 0x16, 0x33, 0x6F, 0x26, 0xE9, 0xC4, 0x12, 
0x0E, 0xEF, 0xAA, 0x28, 0x96, 0xA6, 0x42, 0xB5, 0x84, 0xDC, 0x61, 0x8F, 
0xC4, 0x8B, 0x7F, 0x51, 0xFE, 0x90, 0x17, 0x34, 0x2D, 0x6E, 0x89, 0xEE, 
0x44, 0x0C, 0x2C, 0xE6, 0xBA, 0x02, 0x76, 0xB3, 0xE3, 0xB4, 0xDE, 0x31, 
0x1E, 0x5E, 0xB2, 0xA1, 0x7E, 0xEC, 0xE8, 0x03, };
static embfile_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_FILE;           
   e.res.filename = "/.svn/all-wcprops";        
   e.data = (unsigned char*)data;  
   e.size = sizeof(data);          
   e.file_size = 97;               
   e.mime_type = "application/octet-stream";           
   e.mtime = 1260557763;                  
   e.comp = 1;                    
   e.encrypted = 0;               
}                                  
#ifdef __cplusplus                 
extern "C" {                     
#endif                             
void module_init_ae2b0b0a3526ce49b9ae0c75c5bf755c(void);         
void module_init_ae2b0b0a3526ce49b9ae0c75c5bf755c(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_ae2b0b0a3526ce49b9ae0c75c5bf755c(void);         
void module_term_ae2b0b0a3526ce49b9ae0c75c5bf755c(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
