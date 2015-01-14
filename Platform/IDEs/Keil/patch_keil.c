/***
 * File: patch_keil.c
 * porting iar project to keil, something need to do.
 *
 * Created by JerryYu @ Jan 13rd,2015
 * Ver: 0.1
 * */

#include <string.h>
#include "patch_keil.h"

// exit(1) called by json/debug.c
void exit(int yes){
    
    return;
}
// logging.o
char* __iar_Strstr(char* str1, char* str2){
    return strstr(str1, str2);
}
//mxchipWNET.o
int is_nfc_on(void){
    return 0;
}


