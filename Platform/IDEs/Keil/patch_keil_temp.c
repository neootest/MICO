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
#include "uart.h"

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

// begin added by lean_xiong @2013-12-19
static unsigned char sOsDebugFlag = 1;
void OsSetDebugFlag(unsigned char Flag)
{
	sOsDebugFlag = Flag;
}
// end added by lean_xiong @2013-12-19

static unsigned char IsSwUartActedAsFuartFlag = 0;
void EnableSwUartAsFuart(unsigned char EnableFlag)
{
	IsSwUartActedAsFuartFlag = EnableFlag;
}

//This is used as dummy function in case that appilcation dont define this function.
__weak void SwUartSend(unsigned char* Buf, unsigned int BufLen)
{

}

int fputc(int c, FILE *f) {
// begin added by lean_xiong @2013-12-19
    if(!sOsDebugFlag)
    {
        return c;
    }
// end added by lean_xiong @2013-12-19
    /*
     * putchar is link to uart driver etc.
     */
    if(IsSwUartActedAsFuartFlag)
    {
        if((unsigned char)c == '\n')
        {
            const char lfca[2] = "\r\n";
            SwUartSend((unsigned char*)lfca, 2);

        }
        else
        {
            SwUartSend((unsigned char*)&c, 1);
        }
    }
    else
    {
        if((unsigned char)c == '\n')
        {
            const char lfca[2] = "\r\n";
            FuartSend((unsigned char*)lfca, 2);
        }
        else
        {
            FuartSend((unsigned char*)&c, 1);
        }
    }
    
	return c;
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
//  mico_rtos_suspend_all_thread();
}

USED void _mutex_release(void* mutex)
{
//  mico_rtos_resume_all_thread();
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

