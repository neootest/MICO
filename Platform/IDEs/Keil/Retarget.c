/******************************************************************************/
/* RETARGET.C: 'Retarget' layer for target-dependent low level functions      */
/******************************************************************************/
/* This file is part of the uVision/ARM development tools.                    */
/* Copyright (c) 2005 Keil Software. All rights reserved.                     */
/* This software may only be used under the terms of a valid, current,        */
/* end user licence from KEIL for a compatible version of KEIL software       */
/* development tools. Nothing else gives you the right to use this software.  */
/******************************************************************************/

#include <stdio.h>
#include <time.h>
#include <rt_misc.h>
#include "platform_common_config.h"
#include "MicoPlatform.h"
#include "stm32f2xx.h"

#ifndef __MICROLIB
//if comment semihosting , printf will not work. and must fputc but not putc
#pragma import(__use_no_semihosting_swi)


extern int  sendchar(int ch);  /* in Serial.c */
extern int  getkey(void);      /* in Serial.c */
extern long timeval;           /* in Time.c   */

struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;
#endif

#if 0
int putc(int ch, FILE *f) {
  MicoUartSend( STDIO_UART, &ch, 1 );
  return ch;
}
#else
int fputc(int ch, FILE *f) {
#if 1
  MicoUartSend( STDIO_UART, &ch, 1 );
  return ch;
  //return (sendchar(ch));
#else
        /* Loop until the end of transmission */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
  USART_SendData(USART1, (uint8_t) ch);
    return ch;
#endif
}
#endif
#if 0
int fgetc(FILE *f) {
 // if (MicoUartRecv( STDIO_UART, c, 1, timeout )!=kNoErr)
  //    return -1;
 // else 
  //    return 0;
 // return (sendchar(getkey()));
     while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)
             {}

    return (int)USART_ReceiveData(USART1);
}
#endif

int ferror(FILE *f) {
  /* Your implementation of ferror */
  return EOF;
}


void _ttywrch(int ch) {
 // sendchar (ch);
}


void _sys_exit(int return_code) {
  while (1);    /* endless loop */
}
/*
void HardFault_Handler (){
   printf("HardFault.\n");
}        */ 
