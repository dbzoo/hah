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
0x8D, 0x8E, 0x4B, 0x6E, 0x84, 0x30, 0x10, 0x44, 0xF7, 0xAD, 0xB9, 0x8A, 
0xF1, 0x0F, 0x4C, 0xC3, 0x39, 0x66, 0x35, 0xBB, 0xF6, 0x2F, 0xA0, 0x21, 
0x36, 0x02, 0x33, 0x8C, 0x72, 0xFA, 0x58, 0x91, 0xA2, 0x28, 0x3B, 0x9E, 
0xD4, 0x9B, 0x92, 0xBA, 0x5E, 0x49, 0x01, 0xE0, 0xE7, 0x0D, 0x3A, 0x84, 
0xA9, 0x94, 0x75, 0xE4, 0x7C, 0x99, 0x5F, 0xC1, 0xE6, 0x77, 0xE3, 0xED, 
0x57, 0xCE, 0x8D, 0xCB, 0x9F, 0x7C, 0x7F, 0xA5, 0xDF, 0x94, 0x97, 0xED, 
0x48, 0x4F, 0x7E, 0xEC, 0x61, 0xA3, 0x75, 0xDD, 0xF9, 0x44, 0x13, 0x7F, 
0x2E, 0x39, 0x05, 0x7E, 0x06, 0x5B, 0x13, 0x7E, 0x9E, 0x27, 0xAF, 0x3F, 
0x6B, 0x8D, 0x52, 0xB9, 0x54, 0x09, 0x15, 0x25, 0xC4, 0xC0, 0xA4, 0x64, 
0x62, 0xB8, 0x4B, 0x35, 0x8A, 0x76, 0xD4, 0xA6, 0x41, 0x2D, 0x50, 0xAB, 
0x07, 0x48, 0xB0, 0x5B, 0x28, 0x05, 0xFE, 0x43, 0x83, 0xEF, 0xA4, 0x13, 
0xC4, 0x90, 0xA4, 0x66, 0x6D, 0x2F, 0x14, 0xB3, 0x5E, 0xF5, 0x8C, 0x02, 
0x92, 0xB2, 0x75, 0x8B, 0xC6, 0x16, 0x6E, 0x30, 0x05, 0xF2, 0x73, 0xFA, 
0x68, 0x9E, 0x8B, 0x84, 0x38, 0x2F, 0x01, 0xFE, 0x6C, 0xAA, 0x0A, 0xEF, 
0x12, 0xC7, 0xCE, 0x8C, 0x42, 0x37, 0xE2, 0x87, 0x07, 0x90, 0x75, 0xD2, 
0x63, 0xB4, 0xB1, 0x9E, 0x51, 0xD4, 0x91, 0xEB, 0x8D, 0xA9, 0x5D, 0x1D, 
0x7A, 0xE5, 0x7A, 0x72, 0x97, 0xA6, 0xDE, 0x60, 0x2F, 0x54, 0x8E, 0x7D, 
0x4E, 0x31, 0x5F, 0x57, 0xFB, 0xD6, 0x0D, 0x64, 0x22, 0x56, 0xA1, 0xC1, 
0x16, 0x35, 0xC5, 0x01, 0x11, 0x8D, 0x95, 0x4E, 0xA3, 0xB4, 0x48, 0x17, 
0xD5, 0xDF, };
static embfile_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_FILE;           
   e.res.filename = "/www/component/.svn/entries";        
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
void module_init_31d749bcb472cefbfc4aef7e5eb2e9e7(void);         
void module_init_31d749bcb472cefbfc4aef7e5eb2e9e7(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_31d749bcb472cefbfc4aef7e5eb2e9e7(void);         
void module_term_31d749bcb472cefbfc4aef7e5eb2e9e7(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
