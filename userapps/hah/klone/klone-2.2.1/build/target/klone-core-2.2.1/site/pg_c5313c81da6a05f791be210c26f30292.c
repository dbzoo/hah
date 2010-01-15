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
0xB3, 0x49, 0xC9, 0x2C, 0x53, 0xC8, 0x4C, 0xB1, 0x55, 0xCA, 0xC9, 0x4F, 
0xCF, 0x57, 0xB2, 0xB3, 0xC9, 0x30, 0xB4, 0x73, 0x2C, 0x2D, 0xC9, 0xCF, 
0x4D, 0x2C, 0xC9, 0xCC, 0xCF, 0x53, 0xF0, 0x28, 0x4D, 0xB2, 0xD1, 0x07, 
0x0A, 0xD9, 0xE8, 0x03, 0x95, 0xD9, 0x71, 0x01, 0x00, };
static embfile_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_FILE;           
   e.res.filename = "/www/component/.svn/text-base/heading.kl1.svn-base";        
   e.data = (unsigned char*)data;  
   e.size = sizeof(data);          
   e.file_size = 45;               
   e.mime_type = "application/octet-stream";           
   e.mtime = 1260557763;                  
   e.comp = 1;                    
   e.encrypted = 0;               
}                                  
#ifdef __cplusplus                 
extern "C" {                     
#endif                             
void module_init_c5313c81da6a05f791be210c26f30292(void);         
void module_init_c5313c81da6a05f791be210c26f30292(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_c5313c81da6a05f791be210c26f30292(void);         
void module_term_c5313c81da6a05f791be210c26f30292(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
