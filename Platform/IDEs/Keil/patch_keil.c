/***
 * File: patch_keil.c
 * porting iar project to keil, something need to do.
 *
 * Created by JerryYu @ Jan 13rd,2015
 * Ver: 0.1
 * */


#if defined(__CC_ARM)

#include <string.h>
#include <stdio.h>
#include "MicoPlatform.h"
#include "Common.h"
#include "MicoRTOS.h"

#include <rt_misc.h>

#pragma import(__use_no_semihosting_swi)

struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;
FILE __stderr;


int fgetc(FILE *f) {
  return 0x30;
}


int ferror(FILE *f) {
  /* Your implementation of ferror */
  return EOF;
}


void _ttywrch(int ch) {
  return;
}


void _sys_exit(int return_code) {
label:  goto label;  /* endless loop */
}

int fputc(int ch, FILE *f) {
  MicoUartSend( STDIO_UART, &ch, 1 );
  return ch;
}


char *_sys_command_string(char * cmd, int len) 
{
    cmd = cmd;
    len = len;
    return 0;
}

#ifndef  NO_MICO_RTOS
USED int _mutex_initialize(void* mutex)
{
  return 1;
}

USED void _mutex_acquire(void* mutex)
{
  mico_rtos_suspend_all_thread();
}

USED void _mutex_release(void* mutex)
{
  mico_rtos_resume_all_thread();
}

USED void _mutex_free(void* mutex)
{
}
#endif
// logging.o
char* __iar_Strstr(char* s1, char* s2)
{  
    int n;  
    if (*s2)  
    {  
        while (*s1)  
        {  
            for (n=0; *(s1 + n) == *(s2 + n); n++)  
            {  
                if (!*(s2 + n + 1))  
                    return (char *)s1;  
            }  
            s1++;  
        }  
        return NULL;  
    }  
    else  
        return (char *)s1;  
}  
//mxchipWNET.o
int is_nfc_up(void){
    return 0;
}

int aes_decrypt(void)
{
    return 0;
}

int nfc_config_stop(void)
{
    return 0;
}

int uart_init(void)
{
    return 0;
}

int __data_GetMemChunk(void)
{
	return 0;
}

int wiced_platform_get_rtc_time (){
    return 1;
}
int wiced_platform_set_rtc_time (){
    return 0;
}

#endif 

