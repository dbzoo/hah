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
0x8D, 0x8E, 0x31, 0x6E, 0x84, 0x30, 0x10, 0x45, 0xFB, 0x51, 0xAE, 0x02, 
0xB6, 0x67, 0xC0, 0x36, 0x9C, 0x63, 0xAB, 0x6D, 0xA2, 0xC1, 0x1E, 0x02, 
0x5A, 0x02, 0x08, 0x7B, 0x97, 0x24, 0xA7, 0x0F, 0x5A, 0x25, 0x8A, 0xD2, 
0xF1, 0xCA, 0x57, 0xFC, 0xF7, 0x8D, 0x06, 0x88, 0xE3, 0x06, 0xB5, 0x87, 
0x21, 0xE7, 0xB5, 0x55, 0x6A, 0x1A, 0x1F, 0xD2, 0x2D, 0x1F, 0x65, 0xEC, 
0xBE, 0x96, 0xA5, 0x0C, 0xCB, 0xBB, 0x4A, 0x8F, 0xF9, 0xD7, 0xAA, 0xBC, 
0xDD, 0xE7, 0x9B, 0xBA, 0x27, 0xD9, 0x78, 0x5D, 0x93, 0x1A, 0x78, 0x50, 
0xB7, 0x69, 0x99, 0x45, 0xED, 0xD2, 0x1D, 0x46, 0xED, 0xFB, 0xAE, 0x52, 
0xFE, 0x9C, 0x44, 0xC9, 0x24, 0x6F, 0x3C, 0xE7, 0x53, 0xB3, 0x70, 0x80, 
0x5A, 0x37, 0x85, 0x31, 0x85, 0x6E, 0x2E, 0x06, 0x5B, 0x5D, 0xB5, 0x64, 
0x4B, 0x4F, 0xDA, 0x13, 0x5E, 0xC1, 0x40, 0xB7, 0x49, 0xCE, 0xF0, 0x1F, 
0x6E, 0x62, 0x6D, 0x82, 0xE6, 0xC2, 0xB3, 0xA1, 0xA2, 0x72, 0x1A, 0x8B, 
0x2E, 0xA2, 0x2B, 0x58, 0x3C, 0x63, 0x77, 0xFC, 0x21, 0x5F, 0xC1, 0x0B, 
0x3C, 0xEF, 0xBC, 0x8E, 0x52, 0x86, 0x94, 0xA0, 0x1F, 0x27, 0x81, 0xBF, 
0x1C, 0x1E, 0xC5, 0x8B, 0xF1, 0x6D, 0x6D, 0x5B, 0x4D, 0xA5, 0x7E, 0x72, 
0x85, 0xA0, 0x6B, 0x61, 0xF6, 0xAE, 0x41, 0x43, 0x31, 0xD8, 0xE8, 0x83, 
0x0D, 0xD1, 0x3A, 0x71, 0xB5, 0x44, 0x8E, 0x7D, 0x7F, 0xEA, 0xEB, 0x4F, 
0xF8, 0x7C, 0x95, 0xD8, 0x5B, 0x6A, 0x2C, 0x36, 0x0E, 0x29, 0xF6, 0x0E, 
0x85, 0x90, 0x2C, 0x99, 0xC3, 0x30, 0x7A, 0x13, 0xF0, 0x64, 0xF5, 0x1B, 
};
static embfile_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_FILE;           
   e.res.filename = "/www/style/elegant/.svn/entries";        
   e.data = (unsigned char*)data;  
   e.size = sizeof(data);          
   e.file_size = 466;               
   e.mime_type = "application/octet-stream";           
   e.mtime = 1260702308;                  
   e.comp = 1;                    
   e.encrypted = 0;               
}                                  
#ifdef __cplusplus                 
extern "C" {                     
#endif                             
void module_init_a631168bbabb39a712f23065c75c36b9(void);         
void module_init_a631168bbabb39a712f23065c75c36b9(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_a631168bbabb39a712f23065c75c36b9(void);         
void module_term_a631168bbabb39a712f23065c75c36b9(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
