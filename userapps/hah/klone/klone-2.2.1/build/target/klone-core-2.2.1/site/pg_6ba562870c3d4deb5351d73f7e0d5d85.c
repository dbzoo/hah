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
0xB3, 0xE0, 0x02, 0x00, };
static embfile_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_FILE;           
   e.res.filename = "/.svn/format";        
   e.data = (unsigned char*)data;  
   e.size = sizeof(data);          
   e.file_size = 2;               
   e.mime_type = "application/octet-stream";           
   e.mtime = 1260557763;                  
   e.comp = 1;                    
   e.encrypted = 0;               
}                                  
#ifdef __cplusplus                 
extern "C" {                     
#endif                             
void module_init_6ba562870c3d4deb5351d73f7e0d5d85(void);         
void module_init_6ba562870c3d4deb5351d73f7e0d5d85(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_6ba562870c3d4deb5351d73f7e0d5d85(void);         
void module_term_6ba562870c3d4deb5351d73f7e0d5d85(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
