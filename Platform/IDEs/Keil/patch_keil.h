/***
 * File: patch_keil.h
 * porting iar project to keil, need to do something.
 *
 * */

#ifndef __PATCH_KEIL_H__
#define __PATCH_KEIL_H__

#define __iar_Stderr stderr

extern void exit(int yes);
extern char* __iar_Strstr(char* str1, char* str2);
extern int is_nfc_on(void);

#endif

