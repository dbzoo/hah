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
0xAD, 0x58, 0xDB, 0x8E, 0xDB, 0x36, 0x10, 0x7D, 0xF7, 0x57, 0xB0, 0x30, 
0x02, 0x24, 0x0B, 0x9B, 0xAB, 0x9B, 0x65, 0x5B, 0xFB, 0xD2, 0x22, 0x40, 
0xD0, 0xA2, 0xB7, 0x87, 0xF6, 0x07, 0x28, 0x8B, 0xB6, 0x89, 0x50, 0xA2, 
0x40, 0xD1, 0xEB, 0x75, 0x82, 0xFD, 0xF7, 0xF2, 0x26, 0x89, 0x94, 0xE8, 
0x0D, 0xD2, 0xF6, 0x45, 0xBB, 0xA2, 0x38, 0xC3, 0xB9, 0x9C, 0x39, 0x33, 
0xF4, 0x03, 0xF8, 0xBA, 0x38, 0xB2, 0x46, 0xAC, 0x8F, 0xA8, 0x26, 0xF4, 
0x56, 0xFC, 0x8D, 0xCE, 0xAC, 0x46, 0x2B, 0xF0, 0x13, 0x27, 0x88, 0xAE, 
0xC0, 0xCF, 0x98, 0x3E, 0x63, 0x41, 0x0E, 0x72, 0xA5, 0x43, 0x4D, 0xB7, 
0xEE, 0x30, 0x27, 0xC7, 0x27, 0x23, 0xD1, 0x91, 0x2F, 0xB8, 0x88, 0x71, 
0xFD, 0xB4, 0x78, 0x5D, 0x94, 0xAC, 0xBA, 0x49, 0x4D, 0x25, 0x3A, 0x7C, 
0x3E, 0x71, 0x76, 0x69, 0xAA, 0x62, 0x79, 0x3C, 0xCA, 0x8D, 0x35, 0xE2, 
0x27, 0xD2, 0x14, 0x70, 0x1F, 0xE5, 0x6A, 0xE3, 0x28, 0x97, 0xEF, 0xDF, 
0xF5, 0x72, 0xF0, 0x74, 0x21, 0x52, 0x56, 0xE0, 0x17, 0xB1, 0x46, 0x94, 
0x9C, 0x9A, 0xE2, 0x80, 0x1B, 0x81, 0xF9, 0x93, 0xA7, 0x2E, 0x8E, 0xE3, 
0x41, 0x5D, 0xA4, 0x44, 0xCF, 0x5C, 0x1D, 0xC8, 0x78, 0x85, 0x79, 0xD1, 
0xB0, 0x06, 0x3F, 0x2D, 0x0E, 0x8C, 0x32, 0x5E, 0x2C, 0x0F, 0x87, 0x83, 
0x2F, 0xAA, 0x17, 0xAE, 0xA4, 0x12, 0xE7, 0x22, 0x8E, 0x22, 0x79, 0xEE, 
0x19, 0x93, 0xD3, 0x59, 0x14, 0x30, 0xDA, 0x1B, 0xEB, 0x5B, 0x8E, 0x7D, 
0x03, 0x28, 0x3E, 0x8A, 0x27, 0x2F, 0x2E, 0x1F, 0xD9, 0x85, 0x13, 0xCC, 
0xC1, 0x1F, 0xF8, 0xBA, 0x02, 0xF6, 0x45, 0x89, 0x1E, 0x19, 0xAF, 0xA5, 
0xAC, 0x6B, 0x18, 0x69, 0xDA, 0x8B, 0x80, 0xCD, 0xA5, 0xF6, 0x75, 0x72, 
0x75, 0xA6, 0xFA, 0xBE, 0xBC, 0x72, 0xD4, 0xB6, 0x58, 0x99, 0xDF, 0xB2, 
0x8E, 0x08, 0xC2, 0xE4, 0x47, 0x4C, 0x91, 0x20, 0xCF, 0xD2, 0x89, 0x99, 
0x15, 0xC6, 0xF0, 0x4D, 0x06, 0xD3, 0x3C, 0x55, 0xE6, 0xF6, 0x47, 0x01, 
0x74, 0x11, 0x4C, 0xEB, 0x3B, 0x48, 0x3B, 0x65, 0xC4, 0x42, 0xF1, 0x97, 
0x5F, 0xCF, 0x18, 0x55, 0xFA, 0xB0, 0xA9, 0x22, 0x1B, 0x85, 0x3D, 0x4C, 
0xB3, 0x48, 0xBD, 0xCF, 0xA2, 0xDD, 0xA2, 0xAA, 0x22, 0xCD, 0x69, 0x2D, 
0x58, 0xAB, 0x32, 0xB8, 0x33, 0xC1, 0x82, 0xA4, 0x39, 0xB2, 0x16, 0x9D, 
0x30, 0x18, 0x55, 0x5B, 0x55, 0x39, 0xCC, 0xE3, 0xC4, 0xEC, 0xEA, 0xBF, 
0x2D, 0x29, 0x3B, 0xB1, 0x71, 0x47, 0x06, 0xA3, 0xAD, 0x46, 0x82, 0x31, 
0x26, 0xC9, 0xA3, 0xF6, 0xA5, 0x77, 0x69, 0x5D, 0x32, 0x21, 0x58, 0x5D, 
0xC0, 0x64, 0x6B, 0x95, 0x3C, 0x3E, 0x4C, 0xF4, 0x9C, 0x63, 0xA9, 0xAA, 
0x22, 0x5D, 0x4B, 0xD1, 0xCD, 0x26, 0xFD, 0x75, 0xF1, 0xF0, 0xB8, 0x58, 
0x76, 0x02, 0x89, 0x4B, 0xF7, 0x8B, 0xB4, 0xCC, 0x8D, 0x2A, 0x2A, 0x3B, 
0x46, 0x2F, 0x42, 0x45, 0xD5, 0x71, 0x41, 0x27, 0x42, 0xA5, 0xCA, 0xD8, 
0x90, 0x26, 0x30, 0x37, 0x40, 0x98, 0x27, 0xCB, 0x45, 0xD4, 0xAB, 0x77, 
0x0A, 0x92, 0xE7, 0xB8, 0x5F, 0xB5, 0x6C, 0x85, 0x0F, 0x8C, 0x23, 0x7D, 
0x74, 0x6F, 0x9C, 0x27, 0x53, 0x9C, 0xD9, 0xB3, 0x8E, 0x97, 0x95, 0xD4, 
0x29, 0x9A, 0x4A, 0xCA, 0xF8, 0x63, 0x4E, 0x89, 0x15, 0xAF, 0x71, 0x73, 
0x09, 0x03, 0x65, 0x9A, 0x4E, 0xBB, 0xF9, 0xAF, 0x4B, 0x79, 0x3F, 0x04, 
0x09, 0x4C, 0xF2, 0x8D, 0xDA, 0xAC, 0xA0, 0x35, 0x86, 0x60, 0x54, 0xE2, 
0xA5, 0x3C, 0xCD, 0x75, 0x1E, 0x4C, 0x8D, 0xAD, 0x3B, 0x71, 0xA3, 0xB8, 
0x90, 0xDA, 0x48, 0x05, 0xCC, 0x53, 0xF9, 0x68, 0xFE, 0x1D, 0x36, 0x19, 
0x7D, 0x43, 0x61, 0x69, 0x8B, 0x7E, 0x47, 0xA4, 0xF9, 0x8D, 0x74, 0x62, 
0x05, 0x06, 0x0B, 0xD5, 0xAB, 0x57, 0x35, 0xF6, 0x5C, 0x53, 0x40, 0x9E, 
0x14, 0xA0, 0x64, 0x2A, 0x48, 0x89, 0x83, 0x02, 0xD2, 0x98, 0x58, 0x1D, 
0x29, 0x43, 0xC2, 0x96, 0x0C, 0x95, 0xBB, 0x8C, 0xBD, 0x6B, 0x71, 0x6B, 
0xF1, 0x98, 0x8C, 0x89, 0x62, 0xD0, 0xB5, 0xA8, 0x59, 0xCD, 0x94, 0x3B, 
0xAB, 0xEE, 0x66, 0x14, 0xB0, 0x03, 0x39, 0x96, 0x94, 0x94, 0x1D, 0x3E, 
0x0F, 0x60, 0xD6, 0x11, 0xFE, 0xCF, 0x21, 0xEC, 0xE3, 0x02, 0xB3, 0x8D, 
0xCC, 0x0F, 0xE8, 0x79, 0xF4, 0x2E, 0xDA, 0x42, 0x0E, 0x86, 0xE1, 0xF3, 
0x65, 0x4D, 0x24, 0xD0, 0x5E, 0x8A, 0xD8, 0xF2, 0xDC, 0xD5, 0x14, 0x68, 
0xC9, 0x68, 0x35, 0xC2, 0xA0, 0xAF, 0xC8, 0x3C, 0xC9, 0x47, 0xEA, 0xD1, 
0xE0, 0x88, 0x5C, 0xC0, 0x4D, 0x4E, 0x9B, 0xB0, 0x10, 0xF8, 0x81, 0xD4, 
0x2D, 0xE3, 0x02, 0x35, 0x63, 0x45, 0x45, 0x51, 0xE4, 0x2D, 0x5B, 0xDF, 
0xC7, 0xBA, 0x00, 0xAA, 0xAC, 0x80, 0xDE, 0x37, 0xD3, 0xE1, 0xDA, 0xDB, 
0x48, 0xFE, 0x45, 0x74, 0x6E, 0xF1, 0x26, 0x4B, 0x03, 0x10, 0x34, 0x69, 
0x0C, 0x66, 0xD1, 0xF1, 0x2D, 0x0C, 0xDF, 0x7E, 0xA3, 0xEB, 0x5D, 0x9A, 
0xA6, 0x83, 0x4B, 0xFB, 0xFD, 0x7E, 0xEA, 0xC7, 0x66, 0xB3, 0x01, 0xCB, 
0x24, 0x49, 0xAC, 0x1F, 0xF2, 0x35, 0x14, 0xEC, 0x79, 0x1C, 0x51, 0xDF, 
0x94, 0x27, 0x4E, 0x86, 0x2C, 0x32, 0x9C, 0x12, 0xF0, 0x69, 0x20, 0x1B, 
0x3F, 0x6B, 0xDF, 0x11, 0xA8, 0x41, 0x83, 0xEB, 0x72, 0x9E, 0xE7, 0x93, 
0x4E, 0xEB, 0xB9, 0xBC, 0xDB, 0xED, 0xC0, 0x32, 0xCB, 0x32, 0xEB, 0xB2, 
0x7C, 0x35, 0xBA, 0xA5, 0xDE, 0x95, 0x22, 0x73, 0xDA, 0x7E, 0x94, 0x8E, 
0xC9, 0xB7, 0x7B, 0xFD, 0xCF, 0xAD, 0x63, 0x2B, 0x08, 0xFA, 0x0E, 0xD7, 
0x8D, 0x5D, 0x2C, 0x81, 0x9B, 0x54, 0x61, 0x12, 0x8C, 0x25, 0x62, 0x6B, 
0x63, 0x26, 0x64, 0xBB, 0x82, 0x11, 0xDC, 0x46, 0x30, 0xDB, 0xC6, 0xDF, 
0x21, 0x58, 0x91, 0x67, 0x28, 0x7D, 0x4B, 0xA8, 0xCA, 0x89, 0x63, 0xDB, 
0xD0, 0x3C, 0xB6, 0xDB, 0xEC, 0xDB, 0xD2, 0x3C, 0x2C, 0x9D, 0xF5, 0xD2, 
0x1E, 0x69, 0x0C, 0xF6, 0x9C, 0x13, 0x49, 0x39, 0xE7, 0x54, 0x3E, 0x60, 
0x77, 0x29, 0x6B, 0x22, 0x3E, 0x31, 0x26, 0x9C, 0x9C, 0xCA, 0x21, 0x20, 
0xB2, 0x9C, 0x00, 0xA2, 0xC0, 0xCC, 0x33, 0x78, 0x18, 0xEF, 0x62, 0xC5, 
0x1E, 0xDB, 0x64, 0x13, 0x66, 0xA3, 0x69, 0x16, 0xAB, 0xAA, 0x02, 0x0A, 
0xD0, 0xF6, 0x21, 0x5F, 0xC3, 0xF4, 0x34, 0x56, 0xB3, 0x6A, 0x29, 0x42, 
0x28, 0xAD, 0x2D, 0x3A, 0xE8, 0x33, 0xFB, 0x22, 0x82, 0x9D, 0xFC, 0x20, 
0x57, 0xC2, 0xE9, 0x9E, 0xB4, 0x7E, 0x4D, 0x73, 0x8A, 0xC0, 0x1B, 0xBC, 
0xB6, 0x23, 0x43, 0x0C, 0xF7, 0xFB, 0x74, 0x1C, 0x19, 0xCC, 0x04, 0xE7, 
0xA8, 0x85, 0x14, 0x95, 0x38, 0x9C, 0x9D, 0x78, 0x07, 0xF7, 0xA9, 0x9D, 
0x50, 0x8F, 0x04, 0xD3, 0x4A, 0xCA, 0x80, 0xB9, 0xE0, 0xB0, 0x39, 0x8E, 
0xF3, 0x89, 0xCD, 0xB0, 0xC2, 0x47, 0x74, 0xA1, 0x62, 0x4E, 0xF3, 0x81, 
0x0E, 0x6B, 0xA3, 0xA1, 0x2B, 0xC4, 0x56, 0xDB, 0x64, 0xD6, 0x88, 0x73, 
0x98, 0x46, 0x16, 0x2E, 0xF0, 0x8A, 0x78, 0x63, 0xE2, 0x72, 0x97, 0x80, 
0x0B, 0x3F, 0x65, 0xB6, 0x75, 0xD8, 0x74, 0x2E, 0x8F, 0x8A, 0x71, 0x02, 
0x63, 0xB3, 0xE3, 0xEC, 0xD7, 0x89, 0xA4, 0x4A, 0x0A, 0x98, 0x80, 0xC3, 
0x1E, 0xE1, 0x8E, 0xB0, 0x83, 0x3C, 0xC5, 0x27, 0xDC, 0x54, 0x41, 0x13, 
0x5D, 0xC8, 0xAE, 0xFB, 0x46, 0x37, 0xF4, 0x71, 0xD0, 0x4F, 0x71, 0x81, 
0x91, 0x54, 0xA0, 0x92, 0x62, 0x17, 0xC4, 0x66, 0x92, 0x75, 0xF3, 0xDB, 
0x43, 0xB4, 0x07, 0xD3, 0xC0, 0x52, 0x46, 0x56, 0x70, 0x20, 0xCE, 0x93, 
0x39, 0x2A, 0x44, 0x98, 0x6F, 0x20, 0xDD, 0xC3, 0xB1, 0x8F, 0x7E, 0xDD, 
0x79, 0x64, 0x16, 0xED, 0x63, 0x9C, 0xEF, 0xDC, 0xB1, 0x7C, 0x08, 0x9F, 
0x0E, 0x69, 0x6F, 0xA0, 0xEB, 0xAD, 0x6E, 0x08, 0x92, 0x40, 0xD5, 0xF5, 
0x89, 0x5A, 0x59, 0x83, 0x73, 0xC7, 0x11, 0x15, 0x5B, 0x7F, 0xF8, 0x51, 
0x1C, 0x39, 0x82, 0x72, 0x0B, 0x93, 0x38, 0xF2, 0x26, 0x81, 0xA1, 0xDC, 
0xFB, 0xBD, 0x15, 0xF5, 0x75, 0xB8, 0x89, 0xB4, 0x3B, 0x04, 0x94, 0xC0, 
0xA8, 0x27, 0xBC, 0xA1, 0x53, 0xA6, 0x14, 0x05, 0x5B, 0x92, 0x11, 0xAC, 
0x54, 0x01, 0x90, 0x46, 0x23, 0x7D, 0x3C, 0x65, 0x28, 0x57, 0x3B, 0x51, 
0xDB, 0xF0, 0xD9, 0xD5, 0x4A, 0xFE, 0xC1, 0x3E, 0xD6, 0x7C, 0xBA, 0xDA, 
0xF9, 0xF6, 0x07, 0x66, 0xA8, 0x3B, 0xE3, 0x6B, 0x0C, 0xE3, 0xAD, 0x3B, 
0xC2, 0x4B, 0x5D, 0x71, 0x78, 0x70, 0x0F, 0x0F, 0x48, 0x7A, 0x55, 0x70, 
0x79, 0x91, 0x55, 0xD7, 0xB6, 0x82, 0xB2, 0x2B, 0xE6, 0x07, 0xD4, 0x61, 
0xC7, 0x96, 0xA1, 0xE7, 0xBD, 0x39, 0x96, 0x4F, 0xE9, 0xF8, 0x7E, 0x2D, 
0x05, 0xCB, 0xD3, 0x17, 0xD7, 0xF7, 0xC5, 0xD5, 0x02, 0xD6, 0xB8, 0xEB, 
0xD4, 0xB5, 0x4A, 0xF6, 0x0C, 0xB3, 0xE6, 0x2A, 0x36, 0xC9, 0x8A, 0x61, 
0xB4, 0xDB, 0x7A, 0x97, 0xC0, 0xBE, 0xF0, 0xA4, 0x03, 0x9A, 0xFF, 0x7E, 
0x25, 0x94, 0x9A, 0x2B, 0x06, 0xC5, 0x88, 0x2B, 0xBC, 0x9D, 0x87, 0xCA, 
0x52, 0x57, 0xAD, 0x9E, 0x54, 0xD5, 0xFF, 0xCE, 0x2D, 0x5E, 0xBD, 0xBE, 
0x2E, 0x74, 0xB7, 0xD2, 0x56, 0x86, 0xEF, 0xE3, 0xAF, 0xA3, 0x91, 0x0A, 
0x4A, 0x2F, 0xEB, 0x6F, 0xDC, 0x4D, 0x7B, 0xEB, 0x63, 0xB8, 0x33, 0xF7, 
0xC2, 0x39, 0xD2, 0x42, 0xB7, 0xFE, 0xFF, 0xAD, 0x3D, 0xCD, 0x1A, 0xE2, 
0x6B, 0x38, 0xCA, 0xCE, 0x84, 0x14, 0xC3, 0x2C, 0xDB, 0x4F, 0x4D, 0x1D, 
0xE7, 0x2F, 0xD8, 0xB2, 0xF6, 0xD2, 0x86, 0x6E, 0xDA, 0xF3, 0x69, 0x42, 
0x1E, 0x25, 0xDE, 0x9E, 0x72, 0x2C, 0x58, 0xE7, 0xA3, 0xA7, 0x4D, 0x58, 
0x06, 0x8D, 0x2D, 0x43, 0x27, 0x4C, 0xF2, 0xDD, 0xD4, 0x2F, 0x84, 0xD0, 
0xBF, 0xE5, 0xB9, 0xED, 0x76, 0x6B, 0xC7, 0x6C, 0xFD, 0x90, 0xAF, 0x8E, 
0xD9, 0xB0, 0x44, 0xCE, 0xCD, 0x7E, 0x38, 0x7A, 0xFC, 0xAE, 0x32, 0x77, 
0xFF, 0xA6, 0x19, 0x05, 0x7F, 0x71, 0x19, 0xD4, 0x04, 0xD2, 0xEE, 0xF7, 
0x7C, 0xDD, 0xE2, 0x34, 0x4F, 0x4A, 0x26, 0xE9, 0x03, 0x69, 0xA2, 0xA6, 
0xEF, 0x4C, 0xEE, 0x7D, 0xC0, 0x1D, 0x70, 0xBD, 0x0F, 0xC6, 0x82, 0xFD, 
0xFE, 0x9D, 0xB7, 0xAA, 0xB2, 0xC8, 0xD9, 0x89, 0x4B, 0x20, 0x18, 0x1F, 
0x9F, 0x49, 0x47, 0x4A, 0x42, 0x89, 0xB8, 0x15, 0xFA, 0x5F, 0x8A, 0x9F, 
0x42, 0x39, 0x53, 0xC4, 0x20, 0x2D, 0xB8, 0x16, 0x67, 0x52, 0x55, 0xB8, 
0x09, 0x47, 0xDD, 0x57, 0xAE, 0xE9, 0x2C, 0x1C, 0x24, 0xB9, 0x51, 0x05, 
0xA1, 0x94, 0x6E, 0x7F, 0x0E, 0xD0, 0x8D, 0x5E, 0x1F, 0x7F, 0x4D, 0x2A, 
0x45, 0xE3, 0xB2, 0x41, 0xA4, 0x5B, 0x8E, 0x09, 0x90, 0xD9, 0xA0, 0x3A, 
0x25, 0xE6, 0xA8, 0x22, 0x6C, 0xC4, 0xB3, 0x61, 0x49, 0x68, 0xE7, 0x8D, 
0xC5, 0xE3, 0x03, 0xF8, 0xC4, 0x38, 0xF8, 0xB3, 0x95, 0xFB, 0x40, 0xC9, 
0xD9, 0xB5, 0xC3, 0xBC, 0x03, 0xAC, 0xA1, 0x37, 0xF0, 0xF0, 0xB8, 0xF8, 
0xB1, 0xC6, 0x15, 0x41, 0x00, 0x51, 0x0A, 0x90, 0xEC, 0xF8, 0xEF, 0x6B, 
0xA9, 0xC0, 0xC4, 0x0F, 0x44, 0xED, 0xCB, 0x87, 0xAF, 0xEE, 0x4C, 0xE1, 
0xFE, 0x4A, 0x70, 0x67, 0x5E, 0x08, 0xC4, 0xCF, 0x4C, 0x0A, 0x03, 0x04, 
0x26, 0x93, 0xDF, 0x36, 0x49, 0x67, 0xF3, 0xA2, 0x0F, 0x89, 0x5D, 0xFF, 
0xDB, 0x92, 0xE4, 0x3A, 0x8E, 0x4B, 0x49, 0x9E, 0x72, 0x0B, 0x00, 0x23, 
0x8B, 0x81, 0x24, 0x6A, 0x65, 0x7E, 0x01, 0xE8, 0xED, 0xD3, 0x27, 0x82, 
0x6C, 0xA3, 0xD8, 0x0D, 0x00, 0xA7, 0x6A, 0xC0, 0x85, 0xD3, 0xF7, 0x8F, 
0xA4, 0x96, 0x44, 0xD0, 0x3D, 0x8A, 0x33, 0x67, 0x65, 0x89, 0x39, 0x3C, 
0x91, 0xE3, 0x07, 0x79, 0x23, 0x5F, 0x73, 0xDC, 0x62, 0xA4, 0x81, 0xF2, 
0x0F, };
static embfile_t e;                
static void res_ctor(void)         
{                                  
   e.res.type = ET_FILE;           
   e.res.filename = "/www/style/common.css";        
   e.data = (unsigned char*)data;  
   e.size = sizeof(data);          
   e.file_size = 5406;               
   e.mime_type = "text/css";           
   e.mtime = 1260557763;                  
   e.comp = 1;                    
   e.encrypted = 0;               
}                                  
#ifdef __cplusplus                 
extern "C" {                     
#endif                             
void module_init_dbf0d2540a8391f023db611ef8a3f51d(void);         
void module_init_dbf0d2540a8391f023db611ef8a3f51d(void)          
{                                  
    res_ctor();                    
    emb_register((embres_t*)&e);   
}                                  
void module_term_dbf0d2540a8391f023db611ef8a3f51d(void);         
void module_term_dbf0d2540a8391f023db611ef8a3f51d(void)          
{                                  
    emb_unregister((embres_t*)&e); 
}                                  
#ifdef __cplusplus                 
}                                  
#endif                             
