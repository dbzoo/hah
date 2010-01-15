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
0xBD, 0xD3, 0x4D, 0x6E, 0x83, 0x30, 0x10, 0x05, 0xE0, 0xBD, 0x4F, 0x91, 
0x1E, 0xA0, 0x99, 0x92, 0x14, 0xDA, 0xB0, 0x6E, 0x57, 0x91, 0xBA, 0xEC, 
0xB6, 0x32, 0xE0, 0x06, 0x17, 0x63, 0x23, 0xFF, 0xE0, 0x1C, 0xBF, 0x76, 
0xB2, 0x30, 0xA9, 0x84, 0x32, 0x92, 0xA5, 0x2E, 0x01, 0xCB, 0x1F, 0x6F, 
0x34, 0xEF, 0xB8, 0xD9, 0x95, 0xC4, 0xCC, 0xB2, 0xF6, 0x6D, 0xAD, 0xE9, 
0x57, 0x47, 0xE7, 0x7A, 0x66, 0xDA, 0x70, 0x25, 0x1F, 0x9D, 0x16, 0xE4, 
0x73, 0x53, 0x3D, 0x11, 0x08, 0xDF, 0x41, 0xF0, 0x99, 0x35, 0xEA, 0x0C, 
0x0F, 0xF1, 0x21, 0x1C, 0x81, 0xE7, 0x0A, 0xAC, 0x76, 0x72, 0x00, 0x67, 
0x98, 0xA6, 0xD3, 0x64, 0xA0, 0xA7, 0x3D, 0x0C, 0x42, 0x49, 0x06, 0x9E, 
0x35, 0xE1, 0x0D, 0x78, 0xEF, 0xC9, 0xFB, 0xC7, 0x1B, 0x69, 0xA9, 0x60, 
0xB2, 0xA3, 0x7A, 0x3B, 0x88, 0x82, 0x1C, 0xEF, 0x89, 0x2F, 0xFB, 0x15, 
0xB1, 0x3C, 0x60, 0x44, 0xB8, 0xD1, 0x22, 0xAF, 0xC3, 0x35, 0xCA, 0x22, 
0xF1, 0xB5, 0xB8, 0x05, 0xCA, 0x5E, 0x50, 0x97, 0xE0, 0x6A, 0x1C, 0x95, 
0xDC, 0xFE, 0x98, 0xFB, 0x70, 0x75, 0xC8, 0x82, 0x93, 0x14, 0x5D, 0xEA, 
0xAC, 0x1A, 0xA9, 0x0D, 0xB7, 0x23, 0x53, 0x97, 0x79, 0x23, 0xFF, 0xE3, 
0xC5, 0x5F, 0x30, 0xCC, 0xBA, 0x29, 0x73, 0xE6, 0xFB, 0x57, 0x94, 0x9E, 
0xA8, 0x08, 0x9F, 0xC2, 0xC9, 0x3E, 0x13, 0x46, 0xC6, 0x4E, 0x54, 0x84, 
0xAD, 0xE7, 0xD6, 0x32, 0xEC, 0x92, 0xEF, 0x56, 0xE8, 0x0A, 0x55, 0x2B, 
0x58, 0x62, 0xD7, 0x71, 0xEB, 0x99, 0xB7, 0xCC, 0x64, 0xEA, 0xB8, 0x65, 
0xBB, 0xC1, 0xAE, 0x7B, 0x2E, 0xBF, 0xF9, 0x09, 0x69, 0x17, 0x99, 0xF5, 
0x4E, 0x56, 0xA4, 0xB9, 0xEC, 0xD8, 0x19, 0x27, 0x67, 0x56, 0x2C, 0x49, 
0xD1, 0x9D, 0x68, 0xDB, 0xBB, 0x86, 0x65, 0xCE, 0x1B, 0x99, 0x79, 0x89, 
0x5D, 0xFA, 0xDD, 0x8D, 0x1C, 0x59, 0xED, 0xCC, 0xD0, 0x49, 0x8A, 0xAE, 
0x50, 0xA7, 0x7F, 0x72, 0x93, 0x14, 0xDD, 0x5F, };
static embfile_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_FILE;           
   e.res.filename = "/www/.svn/all-wcprops";        
   e.data = (unsigned char*)data;  
   e.size = sizeof(data);          
   e.file_size = 1702;               
   e.mime_type = "application/octet-stream";           
   e.mtime = 1261043847;                  
   e.comp = 1;                    
   e.encrypted = 0;               
}                                  
#ifdef __cplusplus                 
extern "C" {                     
#endif                             
void module_init_d201cbcac19b043a60f55604754a5373(void);         
void module_init_d201cbcac19b043a60f55604754a5373(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_d201cbcac19b043a60f55604754a5373(void);         
void module_term_d201cbcac19b043a60f55604754a5373(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
