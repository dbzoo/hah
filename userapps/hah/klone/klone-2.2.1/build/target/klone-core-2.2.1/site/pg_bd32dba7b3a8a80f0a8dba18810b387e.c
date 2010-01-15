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
0xAD, 0x58, 0xEB, 0x73, 0xDB, 0x36, 0x12, 0xFF, 0x1C, 0xFE, 0x15, 0x08, 
0x3A, 0x8E, 0x45, 0x9F, 0x24, 0xD2, 0x69, 0xE2, 0xC9, 0x45, 0x8F, 0xA9, 
0xC7, 0x91, 0x7B, 0x9E, 0x4B, 0x32, 0x69, 0x6C, 0x7F, 0xF2, 0x64, 0x34, 
0x10, 0x09, 0x89, 0xA8, 0x29, 0x92, 0x25, 0x41, 0x3D, 0xAE, 0xF5, 0xFF, 
0x7E, 0xBB, 0x78, 0xF0, 0x21, 0xDB, 0x8A, 0x9D, 0x96, 0x33, 0x36, 0x89, 
0x05, 0xF6, 0x87, 0x7D, 0x63, 0xA1, 0xE1, 0xC1, 0x4B, 0xE2, 0x1D, 0x91, 
0xDE, 0x51, 0x8F, 0x2C, 0xD3, 0x90, 0xBF, 0x0F, 0x06, 0x41, 0x1A, 0x8A, 
0x64, 0xF1, 0xBE, 0x94, 0xF3, 0xDE, 0xBB, 0x81, 0x9A, 0x38, 0xF2, 0x9C, 
0x9F, 0x44, 0x12, 0xC4, 0x65, 0xC8, 0x09, 0x5D, 0x8A, 0xE4, 0x22, 0x11, 
0xFD, 0x88, 0x36, 0x68, 0x71, 0xBA, 0x10, 0x49, 0x9B, 0x14, 0xA4, 0x49, 
0x21, 0xDB, 0xA4, 0x52, 0x8A, 0xB8, 0x68, 0x91, 0x86, 0x85, 0xCC, 0x61, 
0xAF, 0x7E, 0x34, 0x76, 0x9C, 0x9F, 0x42, 0x3E, 0x17, 0x09, 0x27, 0x85, 
0xF8, 0x1F, 0x67, 0x79, 0xCE, 0xB6, 0x9D, 0x8D, 0x4B, 0x3A, 0x38, 0x4A, 
0xE7, 0xF8, 0xE9, 0x11, 0xFB, 0x7D, 0xE3, 0x7F, 0x73, 0x5D, 0xA7, 0x90, 
0x4C, 0x8A, 0x80, 0x88, 0x44, 0x92, 0x9C, 0xCF, 0xD2, 0x54, 0x92, 0x11, 
0xF1, 0x07, 0x8E, 0xA5, 0x03, 0x72, 0x19, 0x48, 0xF2, 0xA7, 0xF3, 0x82, 
0x04, 0x11, 0xCB, 0x49, 0x59, 0xF0, 0xFC, 0xE6, 0xE4, 0xCD, 0xB7, 0x81, 
0x25, 0x64, 0xAC, 0x28, 0xD6, 0x61, 0x8B, 0x54, 0xCE, 0x73, 0xFE, 0xC7, 
0xCD, 0xDB, 0x06, 0x41, 0x84, 0xCD, 0xA1, 0x00, 0x95, 0x58, 0x12, 0xF0, 
0x9B, 0x9F, 0xFD, 0x9A, 0xC8, 0x13, 0x36, 0x8B, 0xF9, 0xCD, 0x6B, 0xA0, 
0xDC, 0xC1, 0x0A, 0x51, 0x8B, 0xB0, 0x4A, 0x45, 0x08, 0xB2, 0xB1, 0x10, 
0xEC, 0xD5, 0x71, 0x95, 0x28, 0x30, 0x3F, 0x5D, 0x70, 0x59, 0x74, 0xE8, 
0x22, 0x4D, 0x17, 0x31, 0x0F, 0x58, 0x4C, 0xBB, 0x54, 0x43, 0xC0, 0x87, 
0x4F, 0xBB, 0xB0, 0xA2, 0xAF, 0xC7, 0x5D, 0xA3, 0x6F, 0x4D, 0x71, 0xBB, 
0x60, 0xD5, 0x3E, 0x8C, 0xE7, 0x02, 0x06, 0x83, 0x47, 0xF1, 0x40, 0x6C, 
0x04, 0xF3, 0x3F, 0x9C, 0x6A, 0x3C, 0x18, 0x37, 0xC1, 0x60, 0xF8, 0x54, 
0x24, 0xAB, 0x31, 0x7C, 0xFE, 0x5A, 0x93, 0x11, 0xC4, 0xCE, 0x34, 0x81, 
0x2D, 0xED, 0xC9, 0x72, 0x82, 0x4F, 0xE0, 0x65, 0x84, 0x84, 0x41, 0x4B, 
0x4A, 0x18, 0x3F, 0x15, 0x48, 0xFB, 0xB2, 0x82, 0xD2, 0xC3, 0x26, 0x98, 
0xA6, 0x3C, 0x59, 0x2E, 0x8C, 0x03, 0x78, 0x9F, 0x18, 0x7F, 0xA8, 0x71, 
0x4B, 0x36, 0x24, 0xEC, 0xA2, 0xDD, 0xB5, 0x1D, 0x5F, 0x94, 0xB3, 0xA5, 
0x90, 0xD3, 0x79, 0x9A, 0x2F, 0x2B, 0xE7, 0x63, 0xA8, 0xC2, 0x9A, 0x5C, 
0x8E, 0x8E, 0x21, 0x4C, 0x20, 0x80, 0x30, 0x49, 0x74, 0x18, 0x1D, 0x69, 
0x17, 0x43, 0x10, 0x03, 0x74, 0x09, 0xAB, 0x50, 0xAE, 0x29, 0xCB, 0x17, 
0x1D, 0x33, 0xAE, 0xC2, 0x44, 0x0B, 0x3E, 0xEF, 0x98, 0x98, 0x80, 0x01, 
0x3E, 0xB0, 0x81, 0xA2, 0x76, 0x2A, 0xA0, 0x11, 0x39, 0x3C, 0x3E, 0x24, 
0xAF, 0x5E, 0x91, 0x3A, 0x80, 0x20, 0x73, 0x14, 0xDD, 0x3F, 0x74, 0xC9, 
0x5F, 0x7F, 0x21, 0x07, 0x3C, 0x2D, 0x0E, 0xFF, 0x11, 0x8E, 0xE3, 0x43, 
0xD7, 0x25, 0x86, 0x41, 0xED, 0xA5, 0x6D, 0x97, 0x95, 0x8F, 0xC5, 0xB2, 
0x89, 0xE1, 0x1D, 0x83, 0x03, 0x5B, 0xC3, 0x02, 0x06, 0xEE, 0xCE, 0xB1, 
0xEF, 0x1D, 0x93, 0x40, 0xA0, 0xEE, 0xB3, 0x07, 0x86, 0xB9, 0x32, 0x46, 
0x93, 0xC7, 0xC6, 0xE0, 0x3E, 0xC6, 0x2A, 0xAA, 0xAD, 0x29, 0x71, 0x23, 
0x50, 0x1B, 0x6A, 0x46, 0xB0, 0xCC, 0x70, 0x64, 0xD3, 0xC6, 0x75, 0xB5, 
0x61, 0x49, 0x99, 0x65, 0x3C, 0x87, 0xF9, 0x4E, 0x47, 0x6F, 0xE3, 0xE2, 
0xA4, 0xD6, 0xE0, 0x61, 0x3B, 0xA8, 0x1C, 0x44, 0xA4, 0x5D, 0x0B, 0x10, 
0x6B, 0x01, 0x90, 0x10, 0x6D, 0xA0, 0xD4, 0x17, 0x18, 0x57, 0x46, 0xF0, 
0x5A, 0x90, 0x2A, 0xC7, 0x5A, 0xC9, 0x65, 0x45, 0x7A, 0x78, 0xDF, 0x3A, 
0x63, 0x2B, 0xEE, 0x27, 0x48, 0xB0, 0x6B, 0x78, 0xC8, 0xBD, 0xBD, 0x96, 
0xC7, 0xC4, 0xAD, 0x8C, 0x87, 0x8B, 0x1B, 0xD6, 0xC3, 0x3C, 0xAE, 0x12, 
0x78, 0xBF, 0xB0, 0xBA, 0x00, 0x28, 0x8E, 0x27, 0x99, 0xA9, 0x29, 0xA3, 
0x4E, 0xE9, 0x7D, 0x52, 0x9A, 0xAA, 0x60, 0xE5, 0x34, 0x0C, 0x5A, 0xD2, 
0x98, 0x27, 0x86, 0xE0, 0x92, 0x31, 0xF1, 0x1B, 0xF2, 0x9B, 0xE2, 0xD1, 
0xA8, 0x1A, 0xFB, 0x75, 0xB0, 0xB5, 0xC7, 0xF0, 0x3D, 0x5B, 0x0F, 0x55, 
0x4B, 0xF6, 0x1A, 0x5B, 0x55, 0xA3, 0xCA, 0xDA, 0x6A, 0x79, 0xC3, 0xDC, 
0xAA, 0x36, 0xD5, 0x45, 0xE9, 0x3B, 0x06, 0xD7, 0x95, 0x4D, 0x33, 0x3D, 
0x2D, 0x2E, 0x60, 0x4F, 0x43, 0xD6, 0x55, 0x0C, 0x1F, 0x2D, 0x39, 0x48, 
0xB9, 0xBA, 0x81, 0xF2, 0x40, 0xFE, 0xA4, 0x1E, 0x97, 0x81, 0x07, 0x58, 
0xB2, 0x1F, 0x7A, 0x1B, 0x96, 0xC1, 0x4E, 0x86, 0x07, 0xBE, 0x1A, 0xFB, 
0x93, 0xCF, 0xD7, 0x1F, 0x3F, 0xDE, 0x0D, 0x0C, 0x0A, 0x54, 0xC8, 0x2C, 
0x4F, 0x03, 0x5E, 0x14, 0x1D, 0x84, 0x72, 0xF5, 0x96, 0x77, 0x8E, 0x77, 
0xF4, 0x8F, 0x3C, 0x9E, 0x73, 0x30, 0x76, 0x86, 0x07, 0x0E, 0x51, 0x85, 
0x17, 0xCE, 0x65, 0x42, 0xBE, 0x4E, 0x7E, 0xBB, 0xBE, 0xF8, 0x3A, 0x99, 
0x9E, 0x5E, 0x5F, 0xFD, 0xA7, 0x73, 0x79, 0xF6, 0xF5, 0xE2, 0xCB, 0xD5, 
0xF4, 0xF3, 0xE9, 0xA7, 0x89, 0x8B, 0x93, 0x4D, 0xA7, 0xB0, 0x40, 0x8A, 
0x34, 0xD9, 0xE7, 0x15, 0xBD, 0x82, 0x2A, 0xCE, 0xEA, 0x8C, 0xC7, 0x01, 
0xD8, 0xCB, 0x70, 0xD7, 0x4E, 0xD2, 0x84, 0x2E, 0x3D, 0xCD, 0xB2, 0x78, 
0x4B, 0x5D, 0xAC, 0xA8, 0x3E, 0x5A, 0x93, 0x58, 0x3B, 0xD4, 0x27, 0xC5, 
0xC0, 0x10, 0x5B, 0x98, 0x77, 0x4A, 0x95, 0x48, 0x2E, 0x63, 0x7C, 0xC1, 
0x0C, 0xBC, 0x96, 0x5C, 0x32, 0x12, 0x49, 0x99, 0xF5, 0x40, 0x24, 0xB1, 
0x1A, 0xD1, 0xB3, 0x34, 0x91, 0x3C, 0x91, 0xBD, 0xAB, 0x6D, 0xC6, 0x29, 
0x6A, 0x83, 0xA3, 0x11, 0x65, 0xB0, 0xA7, 0x08, 0x18, 0x0A, 0xE0, 0x6D, 
0x10, 0xE2, 0x5F, 0x9B, 0x65, 0x3C, 0x50, 0x7A, 0x16, 0x5C, 0x8E, 0x44, 
0x91, 0xF6, 0xDE, 0xBD, 0x7B, 0xFB, 0xEF, 0xDE, 0x31, 0xF5, 0x00, 0xB5, 
0x08, 0x72, 0x91, 0x49, 0x22, 0x01, 0x63, 0x44, 0x25, 0xDF, 0x48, 0xEF, 
0x77, 0xB6, 0x62, 0x9A, 0x4A, 0x49, 0x91, 0x07, 0x23, 0xE8, 0xEC, 0x96, 
0x4B, 0x88, 0x9C, 0xDF, 0x0B, 0x3A, 0x1E, 0x7A, 0x7A, 0x06, 0x18, 0x63, 
0x91, 0xDC, 0x36, 0xD9, 0x82, 0xA2, 0xA0, 0xA0, 0x44, 0x3C, 0xA2, 0x85, 
0xDC, 0xC6, 0xBC, 0x88, 0x38, 0x07, 0x80, 0x28, 0xE7, 0x73, 0x43, 0xF1, 
0x82, 0x32, 0xCF, 0x41, 0x42, 0x4F, 0x8D, 0xFA, 0xB8, 0x1E, 0x05, 0x78, 
0xD9, 0xEB, 0xDD, 0x88, 0x39, 0xB9, 0x98, 0x7C, 0x1B, 0xFF, 0x5D, 0xCC, 
0xA9, 0xB0, 0xB0, 0xC3, 0x97, 0x37, 0x3C, 0x09, 0xC5, 0xFC, 0x5B, 0xAF, 
0x07, 0x5B, 0x48, 0x21, 0x63, 0x3E, 0xFE, 0x28, 0x56, 0xD0, 0x36, 0x6E, 
0x86, 0x9E, 0x1E, 0x3A, 0xDF, 0xD1, 0x7D, 0xEC, 0xCC, 0xCB, 0x44, 0xFB, 
0x55, 0xA6, 0x53, 0xED, 0xB2, 0xCE, 0x39, 0x3A, 0xF1, 0xBC, 0x5F, 0xB0, 
0x15, 0x9F, 0xCE, 0x4A, 0x29, 0xC1, 0x2C, 0x2B, 0x16, 0x97, 0x78, 0xF8, 
0xD0, 0x4B, 0xB6, 0x82, 0xD6, 0x96, 0x0E, 0x60, 0x5E, 0xBB, 0x5F, 0x4F, 
0x8D, 0x4C, 0x10, 0x0C, 0x1C, 0x74, 0xCC, 0x16, 0x20, 0xB0, 0x73, 0x60, 
0x61, 0x38, 0x59, 0x81, 0xE0, 0x9D, 0xB5, 0x48, 0xC2, 0x74, 0xDD, 0x85, 
0x8E, 0x9A, 0x41, 0x4D, 0xB1, 0x5B, 0x9A, 0x06, 0xA2, 0x88, 0xD2, 0xF5, 
0x34, 0x66, 0x5B, 0x9E, 0x4F, 0x41, 0xBA, 0x4E, 0x98, 0x06, 0xE5, 0x12, 
0x98, 0xFA, 0x50, 0x3F, 0x57, 0x02, 0xF2, 0xC8, 0x36, 0x8E, 0xE4, 0x50, 
0x84, 0x59, 0x74, 0xD8, 0x25, 0xC7, 0x18, 0x69, 0xC3, 0x83, 0x51, 0xE3, 
0x40, 0x27, 0x07, 0x63, 0x9D, 0x68, 0xF0, 0xBF, 0xE1, 0x3E, 0xCF, 0x44, 
0xD5, 0x2C, 0x0D, 0xB7, 0x24, 0x88, 0xA1, 0x96, 0x8D, 0xE8, 0xA2, 0x14, 
0x14, 0xED, 0x12, 0x8A, 0x15, 0x11, 0xE1, 0x88, 0xAE, 0x73, 0x86, 0xA7, 
0x20, 0xD0, 0x2A, 0x92, 0x89, 0xB2, 0x26, 0x09, 0x81, 0xD4, 0x22, 0xC8, 
0xBD, 0x5F, 0x6C, 0xA3, 0x0F, 0x31, 0x93, 0xA5, 0x09, 0x7A, 0x06, 0xA7, 
0xB1, 0xE5, 0xBF, 0x8D, 0x8F, 0x0F, 0x9A, 0xE0, 0xA0, 0x48, 0x09, 0x4C, 
0xA4, 0x45, 0xF8, 0xC4, 0x44, 0x82, 0x44, 0x32, 0x2C, 0xE3, 0x16, 0xF1, 
0xA3, 0x28, 0xA4, 0x9A, 0x20, 0x10, 0x24, 0x56, 0x5E, 0xE3, 0x7C, 0x08, 
0xCB, 0x22, 0x63, 0xC9, 0xF8, 0xB4, 0x94, 0xE9, 0x52, 0xC5, 0x3D, 0xE8, 
0x89, 0x04, 0x95, 0x59, 0x2D, 0xFC, 0xCB, 0x72, 0xA6, 0x51, 0xDA, 0x3B, 
0x00, 0xD9, 0x6C, 0xA0, 0xCA, 0x87, 0x7A, 0x3E, 0x4D, 0x3E, 0x5F, 0x4F, 
0x2F, 0xAE, 0x26, 0x9F, 0x3A, 0x94, 0x55, 0xC0, 0xA8, 0x05, 0x54, 0x38, 
0xCC, 0xBC, 0x3C, 0x8D, 0x69, 0x95, 0xBC, 0xCD, 0xD5, 0x60, 0xA2, 0xB9, 
0x58, 0xD4, 0x2B, 0x61, 0x50, 0xE6, 0xAA, 0xF5, 0x38, 0x18, 0xDB, 0xE5, 
0xA0, 0x84, 0x11, 0xFA, 0x8C, 0xC1, 0xC9, 0x14, 0xB2, 0xDC, 0x88, 0x3C, 
0xF4, 0x60, 0xE6, 0x01, 0x50, 0xB9, 0x16, 0x52, 0xF2, 0xDC, 0xA0, 0x5E, 
0xE9, 0x11, 0x62, 0x3E, 0x28, 0x70, 0xC6, 0x82, 0xA8, 0x9C, 0x71, 0xB3, 
0xFA, 0x8B, 0x1E, 0x55, 0x27, 0xCA, 0x83, 0xCD, 0x9E, 0x39, 0x45, 0x1A, 
0x20, 0x0B, 0xF0, 0x7E, 0x64, 0x20, 0x7E, 0xC5, 0x6F, 0x6A, 0x0A, 0xB6, 
0x51, 0x63, 0xE8, 0x95, 0xB1, 0xFD, 0x02, 0x1B, 0x6B, 0xE7, 0x28, 0xF1, 
0x41, 0xA6, 0x36, 0x14, 0xD4, 0x9D, 0x32, 0x33, 0x50, 0x97, 0xF8, 0x5D, 
0x59, 0xC3, 0x80, 0x18, 0x04, 0xF3, 0xAA, 0xDF, 0x95, 0xEF, 0x74, 0x5C, 
0xEC, 0x86, 0x61, 0x81, 0x34, 0x2C, 0xA3, 0x24, 0x61, 0x4B, 0xC8, 0x34, 
0x9B, 0x17, 0x94, 0xE8, 0x0C, 0x1C, 0x51, 0xCC, 0x85, 0x46, 0xDD, 0x87, 
0x64, 0xA0, 0x04, 0x0A, 0x69, 0x94, 0x02, 0x48, 0x96, 0x6A, 0x97, 0x8B, 
0x04, 0xCE, 0x4D, 0x53, 0x05, 0x22, 0x11, 0x86, 0x3C, 0xA1, 0x16, 0x4F, 
0x17, 0x6A, 0x9D, 0xE9, 0xAA, 0x5A, 0x3D, 0xBE, 0xD6, 0x9C, 0x0F, 0xB8, 
0x68, 0x96, 0xE3, 0xFF, 0xE8, 0xF5, 0x58, 0x5F, 0xA1, 0x88, 0xF5, 0x31, 
0x01, 0xDD, 0x25, 0xE4, 0x42, 0x01, 0xD9, 0xF7, 0x7A, 0x3C, 0x34, 0x3A, 
0xCE, 0x05, 0x8F, 0x43, 0xB0, 0x90, 0x32, 0x47, 0xCC, 0x17, 0xB0, 0x74, 
0x97, 0x11, 0xCC, 0xAA, 0xE9, 0x8E, 0x89, 0x67, 0x93, 0x00, 0x85, 0xC6, 
0xA3, 0x8D, 0x48, 0x37, 0x33, 0x31, 0x9B, 0xF1, 0x98, 0x8E, 0x2F, 0xD5, 
0xE9, 0x6E, 0xCC, 0x62, 0xAC, 0xFA, 0xC2, 0x28, 0x61, 0x31, 0x20, 0x58, 
0x78, 0x0E, 0x39, 0x9A, 0x52, 0xA3, 0x97, 0x19, 0x98, 0xEA, 0x75, 0x6C, 
0x15, 0x34, 0x8D, 0x3E, 0x49, 0x21, 0xC5, 0x45, 0x70, 0x0B, 0x9C, 0xED, 
0xEA, 0x24, 0x23, 0x51, 0xD4, 0x95, 0x08, 0x6E, 0xE3, 0xD0, 0x64, 0x80, 
0xFD, 0xF7, 0x44, 0x1C, 0x78, 0x03, 0x8E, 0x25, 0x1E, 0xDC, 0x72, 0x74, 
0xA9, 0xFE, 0x40, 0x8E, 0x3B, 0x98, 0xF0, 0xC6, 0x13, 0xC5, 0xF0, 0x2A, 
0x99, 0x15, 0x99, 0x4E, 0xB3, 0xE7, 0xC9, 0xED, 0xFF, 0x98, 0xDC, 0x73, 
0x16, 0x17, 0xFB, 0x04, 0xF7, 0xBF, 0x2F, 0xF8, 0x07, 0x51, 0x20, 0x87, 
0xD3, 0x48, 0x8C, 0x87, 0x9C, 0xA6, 0x02, 0x19, 0x77, 0xA5, 0x8F, 0x45, 
0x81, 0x2D, 0x1E, 0xA6, 0xA6, 0xD9, 0x18, 0x78, 0x6E, 0x10, 0x6C, 0x4E, 
0xBF, 0x90, 0xEB, 0x8B, 0x0F, 0x75, 0x96, 0x5A, 0x4B, 0x2E, 0xD9, 0x06, 
0xE2, 0x6B, 0x21, 0xA3, 0x11, 0x7D, 0x43, 0xD5, 0xAF, 0x2D, 0xEA, 0x43, 
0x9B, 0x0D, 0xEF, 0x33, 0xD6, 0x96, 0xF6, 0x44, 0xC1, 0x6B, 0x13, 0x66, 
0x50, 0x9A, 0xCC, 0xE2, 0x32, 0x07, 0xE9, 0x8B, 0x4D, 0x28, 0x16, 0xC2, 
0xD8, 0xF0, 0x10, 0x36, 0x39, 0x74, 0x31, 0x03, 0xD4, 0x1E, 0x58, 0xCF, 
0xAC, 0x24, 0x21, 0x9F, 0xB3, 0x32, 0x86, 0x6C, 0xEB, 0x7C, 0xD0, 0x5F, 
0xEF, 0x09, 0xFE, 0x5A, 0xD1, 0x25, 0x5F, 0x59, 0xB2, 0xE0, 0x38, 0xF0, 
0x7D, 0xD2, 0x23, 0xE7, 0xF0, 0xB8, 0x75, 0xF1, 0xB6, 0x65, 0xE0, 0x07, 
0xF4, 0xBD, 0x30, 0x77, 0xA1, 0xBD, 0x4A, 0xFF, 0xEC, 0x5B, 0xAD, 0x5F, 
0xBF, 0xB5, 0x6A, 0x57, 0xD7, 0xA9, 0x5D, 0xDD, 0xAB, 0xEB, 0x1A, 0x18, 
0xE0, 0x89, 0x2A, 0x56, 0xBF, 0xA0, 0xFC, 0x7D, 0x9D, 0xAE, 0x21, 0x87, 
0x51, 0xC0, 0xBD, 0xFA, 0x9C, 0xD4, 0x5E, 0xAC, 0xA2, 0x5F, 0xDD, 0xB8, 
0xEE, 0xF9, 0x11, 0x6F, 0x70, 0x56, 0x8F, 0x1F, 0x94, 0xE8, 0x0B, 0x5E, 
0x7F, 0xD2, 0x3C, 0x7C, 0xB6, 0x44, 0xE6, 0xFE, 0xD4, 0xDC, 0xFB, 0xB9, 
0xC6, 0xC8, 0x42, 0x26, 0x39, 0x39, 0x57, 0xBD, 0x79, 0x12, 0x6C, 0x6D, 
0x61, 0x7B, 0x40, 0x80, 0xB7, 0x76, 0xFF, 0xCA, 0xC1, 0xFA, 0x46, 0x74, 
0xCF, 0x22, 0xEA, 0x96, 0xD5, 0x8C, 0x6D, 0x58, 0x20, 0xC2, 0x69, 0x8E, 
0xF1, 0xA9, 0xC3, 0xFB, 0xC4, 0xEF, 0xBE, 0x3B, 0x79, 0xE3, 0xFB, 0x10, 
0xE7, 0x3B, 0xFB, 0xEB, 0xA0, 0x7F, 0xF1, 0xDD, 0x78, 0x00, 0x04, 0xF8, 
0xEB, 0x29, 0x14, 0x28, 0xCA, 0xC1, 0xFD, 0xB0, 0x18, 0x7A, 0x75, 0x1D, 
0x18, 0x32, 0xD3, 0xE9, 0x62, 0xCF, 0xFF, 0xDE, 0xF3, 0xD6, 0xEB, 0x75, 
0x5F, 0x5F, 0xAD, 0xFA, 0xD0, 0x5D, 0x79, 0x81, 0x39, 0x1B, 0x3C, 0x68, 
0x81, 0xB0, 0xFF, 0x22, 0x50, 0xEA, 0xE1, 0xD2, 0x32, 0xA2, 0xD3, 0x59, 
0xCC, 0x92, 0x5B, 0x3A, 0x9E, 0x84, 0x42, 0x92, 0x6D, 0x5A, 0xE6, 0x24, 
0xA8, 0x8E, 0x11, 0xD6, 0xDE, 0xA2, 0x3A, 0x69, 0xF5, 0x91, 0xE5, 0xB4, 
0xBC, 0xA0, 0xCE, 0xBE, 0xF3, 0x34, 0x95, 0xBA, 0x03, 0x6C, 0x9E, 0x7C, 
0xE6, 0x40, 0xB4, 0xA7, 0x64, 0xDD, 0x0D, 0x57, 0x76, 0x85, 0x66, 0x98, 
0x57, 0xC7, 0x5D, 0xA3, 0xF6, 0xD6, 0xAD, 0x34, 0xDA, 0xB4, 0x8F, 0x67, 
0xB7, 0x7B, 0xEF, 0x60, 0x85, 0xAB, 0x24, 0xF6, 0xF9, 0x06, 0xEA, 0x0C, 
0xB3, 0x2E, 0x26, 0x67, 0x11, 0xBA, 0x42, 0xDF, 0x19, 0x2A, 0x63, 0x01, 
0x7B, 0xDD, 0x2E, 0xDC, 0xEB, 0x1A, 0xE6, 0xD0, 0x54, 0xCB, 0xFF, 0x8A, 
0x38, 0x46, 0x0D, 0xEC, 0xA2, 0x87, 0x7A, 0x54, 0xFC, 0x19, 0xAF, 0x2C, 
0x44, 0x32, 0x4F, 0xAB, 0x36, 0x75, 0xB7, 0x21, 0xC1, 0x3E, 0x59, 0xB5, 
0xCD, 0xEA, 0x4E, 0xF6, 0x7F, };
static embfile_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_FILE;           
   e.res.filename = "/www/calendar.kl1~";        
   e.data = (unsigned char*)data;  
   e.size = sizeof(data);          
   e.file_size = 5957;               
   e.mime_type = "application/octet-stream";           
   e.mtime = 1260701065;                  
   e.comp = 1;                    
   e.encrypted = 0;               
}                                  
#ifdef __cplusplus                 
extern "C" {                     
#endif                             
void module_init_bd32dba7b3a8a80f0a8dba18810b387e(void);         
void module_init_bd32dba7b3a8a80f0a8dba18810b387e(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_bd32dba7b3a8a80f0a8dba18810b387e(void);         
void module_term_bd32dba7b3a8a80f0a8dba18810b387e(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
