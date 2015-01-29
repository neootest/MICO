/**
******************************************************************************
* @file    hardfault.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide debug information in hardfault.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2014 MXCHIP Inc.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy 
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights 
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is furnished
*  to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
*  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************
*/ 

#include "stdio.h"
#include "MicoRTOS.h"
#include "MicoDefaults.h"
#include "MicoPlatform.h"
#include "platform_common_config.h"
#include "platform_mico.h"


//void hard_fault_handler_c (unsigned int * hardfault_args)
void hard_fault_handler_c(unsigned long* irqs_regs, unsigned long* user_regs)
{
	DBG("\n>>>>>>>>>>>>>>[");
	switch(__get_IPSR())
	{
		case	3:
			DBG("Hard Fault");
			break;

		case	4:
			DBG("Memory Manage");
			break;

		case	5:
			DBG("Bus Fault");
			break;

		case	6:
			DBG("Usage Fault");
			break;

		default:
			DBG("Unknown Fault %d", __get_IPSR());
			break;
	}
	DBG(",corrupt,dump registers]>>>>>>>>>>>>>>>>>>\n");

	DBG("R0  = 0x%08X\n", irqs_regs[0]);
	DBG("R1  = 0x%08X\n", irqs_regs[1]);
	DBG("R2  = 0x%08X\n", irqs_regs[2]);
	DBG("R3  = 0x%08X\n", irqs_regs[3]);

	DBG("R4  = 0x%08X\n", user_regs[0]);
	DBG("R5  = 0x%08X\n", user_regs[1]);
	DBG("R6  = 0x%08X\n", user_regs[2]);
	DBG("R7  = 0x%08X\n", user_regs[3]);
	DBG("R8  = 0x%08X\n", user_regs[4]);
	DBG("R9  = 0x%08X\n", user_regs[5]);
	DBG("R10 = 0x%08X\n", user_regs[6]);
	DBG("R11 = 0x%08X\n", user_regs[7]);

	DBG("R12 = 0x%08X\n", irqs_regs[4]);
	DBG("SP  = 0x%08X\n", &irqs_regs[8]);
	DBG("LR  = 0x%08X\n", irqs_regs[5]);
	DBG("PC  = 0x%08X\n", irqs_regs[6]);
	DBG("PSR = 0x%08X\n", irqs_regs[7]);

	DBG("BFAR = 0x%08X\n", (*((volatile unsigned long*)(0xE000ED38))));
	DBG("CFSR = 0x%08X\n", (*((volatile unsigned long*)(0xE000ED28))));
	DBG("HFSR = 0x%08X\n", (*((volatile unsigned long*)(0xE000ED2C))));
	DBG("DFSR = 0x%08X\n", (*((volatile unsigned long*)(0xE000ED30))));
	DBG("AFSR = 0x%08X\n", (*((volatile unsigned long*)(0xE000ED3C))));
	//DBG("Terminated@%u ms\n", auxtmr_count_get());
	/*
	#ifdef	DEBUG
	if(*(unsigned long*)0x20000000 != 0xA5A5A5A5)
	{
		DBG("Error:System Stack Overflow\n");
		return;
	}
	#endif //DEBUG
	*/
	/*
	 * indefinitely deadloop
	 */
  while (1);
}
