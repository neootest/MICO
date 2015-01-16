/***
 * File: patch_keil.c
 * porting iar project to keil, something need to do.
 *
 * Created by JerryYu @ Jan 13rd,2015
 * Ver: 0.1
 * */

#include <string.h>

#if defined(__CC_ARM)

#include <stdio.h>
#include <rt_misc.h>
#include "platform_common_config.h"
#include "MicoPlatform.h"
#include "stm32f2xx.h"
#include "patch_keil.h"
#if 0
#if defined(DEBUG)
#if defined(DEBUG_SEMIHOSTING)
#define ITM_Port8(n)    (*((volatile unsigned char *) (0xE0000000 + 4 * n)))
#define ITM_Port16(n)   (*((volatile unsigned short *) (0xE0000000 + 4 * n)))
#define ITM_Port32(n)   (*((volatile unsigned long *) (0xE0000000 + 4 * n)))

#define DEMCR           (*((volatile unsigned long *) (0xE000EDFC)))
#define TRCENA          0x01000000

/* Write to SWO */
void _ttywrch(int ch)
{
	if (DEMCR & TRCENA) {
		while (ITM_Port32(0) == 0) {}
		ITM_Port8(0) = ch;
	}
}

#else

#endif /* defined(DEBUG_SEMIHOSTING) */
#endif /* defined(DEBUG_ENABLE) */
#if 0
struct __FILE {
	int handle;
};

FILE __stdout;
FILE __stdin;
FILE __stderr;

void *_sys_open(const char *name, int openmode)
{
	return 0;
}
#endif
int fputc(int c, FILE *f)
{
#if defined(DEBUG)
#if defined(DEBUG_SEMIHOSTING)
	_ttywrch(c);
#else
  //MicoUartSend( STDIO_UART, &c, 1 );
  
  USART_SendData(USART1, (uint8_t) c);
        /* Loop until the end of transmission */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
    {}
  return c;
#endif
#endif
	return 0;
}

int fgetc(FILE *f)
{
#if defined(DEBUG) && !defined(DEBUG_SEMIHOSTING)
//    int* c;
//  if (MicoUartRecv( STDIO_UART, c, 1, 0)!=kNoErr)
	return 1;
#else
	return 0;
#endif
}

int ferror(FILE *f)
{
	return EOF;
}

void _sys_exit(int return_code)
{
label:
	__WFI();
	goto label;	/* endless loop */
}
#endif 

// logging.o
char* __iar_Strstr(char* str1, char* str2){
    return strstr(str1, str2);
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

int __iar_Stderr;

int wiced_platform_get_rtc_time (){
    return 1;
}
int wiced_platform_set_rtc_time (){
    return 0;
}

#endif 

