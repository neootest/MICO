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

#pragma import(__use_no_semihosting_swi)

#if 0
extern int  sendchar(int ch);  /* in Serial.c */
extern int  getkey(void);      /* in Serial.c */
extern long timeval;           /* in Time.c   */
#endif

struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;


int fputc(int ch, FILE *f) {
 // MicoUartSend( STDIO_UART, &ch, 1 );
 // return ch;
  //return (sendchar(ch));
  USART_SendData(USART1, (uint8_t) ch);
        /* Loop until the end of transmission */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
    {}
    return ch;
}

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


int ferror(FILE *f) {
  /* Your implementation of ferror */
  return EOF;
}


void _ttywrch(int ch) {
    USART_SendData(USART1, (uint8_t) ch);
        /* Loop until the end of transmission */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
    {}
 // sendchar (ch);
}


void _sys_exit(int return_code) {
  while (1);    /* endless loop */
}
/*
void HardFault_Handler (){
   printf("HardFault.\n");
}        */ 
