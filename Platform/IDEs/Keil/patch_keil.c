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
#include <rt_misc.h>
#include "platform_common_config.h"
#include "MicoPlatform.h"
#include "stm32f2xx.h"
#include "patch_keil.h"

extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);

size_t strnlen(const char *s, size_t count)    
{    
        const char *sc;    
            
            for (sc = s; count-- && *sc != '\0'; ++sc)    
                        /* nothing */;    
                return sc - s;    
} 

int fputc(int ch, FILE *f) {
#if 1
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, (uint8_t) ch);
    return ch;
#else 
  MicoUartSend( STDIO_UART, &ch, 1 );
  return ch;
#endif 
}
#ifdef __MICROLIB
void exit(int x)
{
    x = x;
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

