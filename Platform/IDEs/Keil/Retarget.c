/******************************************************************************/
/* RETARGET.C: 'Retarget' layer for target-dependent low level functions      */
/******************************************************************************/
/* This file is part of the uVision/ARM development tools.                    */
/* Copyright (c) 2005 Keil Software. All rights reserved.                     */
/* This software may only be used under the terms of a valid, current,        */
/* end user licence from KEIL for a compatible version of KEIL software       */
/* development tools. Nothing else gives you the right to use this software.  */
/******************************************************************************/
#if defined(__CC_ARM)

#include <stdio.h>
#include <time.h>
#include <rt_misc.h>
#include "platform_common_config.h"
#include "MicoPlatform.h"

#ifndef __MICROLIB
//if comment semihosting , printf will not work. and must fputc but not putc
#pragma import(__use_no_semihosting_swi)

struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;
FILE __stderr;

int fputc(int ch, FILE *f) {

  MicoUartSend( STDIO_UART, &ch, 1 );
  return ch;
}

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
  while (1);    /* endless loop */
}

char *_sys_command_string (char *cmd, int len) 
{
    cmd = cmd;
    len = len;
    return 0;
}
#endif


#endif 
