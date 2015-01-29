/**
******************************************************************************
* @file    platform_mico.h 
* @author  Jerry Yu
* @version 0.0.1
* @date    Jan-22nd,2015
* @brief   This file provide all the headers of functions for platform_mico
******************************************************************************
*
*  The MIT License
*  Copyright © 2015 MXCHIP Inc.
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




#ifndef __PLATFORM_MICO_H__
#define __PLATFORM_MICO_H__

//#include "ap80a0.h" //TBD!
#include "MicoRTOS.h"
#include "type.h"
//#include "app_config.h"
#include "rtc.h"
#include "delay.h"
//#include "os.h"
#include "timeout.h"
#include "chip_info.h"
#include "uart.h"
#include "wakeup.h"
#include "watchdog.h"
#include "clk.h"
#include "spi_flash.h"
#include "cache.h"
#include "gpio.h"
#include "dac.h"
#include "audio_adc.h"
#include "timer.h"
#include "adc.h"
#include "ir.h"
#include "host_hcd.h"
#include "watchdog.h"
#include "mixer.h"
#include "eq.h"
#include "lcd_seg.h"
#include "ap80xx_flash.h"


#if defined(DEBUG)
	#define	DBG(format, ...)		printf(format, ##__VA_ARGS__)
#else
	#define	DBG(format, ...)
#endif

typedef enum {
    Fuart,
    Buart,
}usart_p;

typedef struct {
    usart_p             usart;      //which usart
    uint8_t             rxPin;      //0, 1, 2, 0xFF, detail pls see gpio.h
    uint8_t             txPin;      //0, 1, 2, 0xFF, detail pls see gpio.h
    uint8_t             ctsPin;     //0, 1, 2, 0xFF, detail pls see gpio.h
    uint8_t             rtsPin;     //0, 1, 2, 0xFF, detail pls see gpio.h
    bool                usart_irq_en;  //Enable FuartInterrupt or BuarInterrupt
    bool                exFifoEn;
#ifdef MICO_USART_DMA_EN
    uint8_t             dmaCH;
#endif
} platform_uart_mapping_t;

// Modules' clock used by MICO
#define MICO_MODULE_CLK_SWITCH      FUART_CLK_EN |\
                                    PHUB_CLK_EN |\
                                    TIMER0_CLK_EN    
//#define MICO_MODULE_CLK_SWITCH          ALL_MODULE_CLK_SWITCH

#define MICO_MODULE_CLK_GATE_SWITCH     CACHE_CLK_GATE_EN | CM3_CLK_GATE_EN \
             | XMEM_CLK_GATE_EN | GPIO_CLK_GATE_EN | ROM_CLK_GATE_EN | FSHC_CLK_GATE_EN
//#define MICO_MODULE_CLK_GATE_SWITCH     ALL_MODULE_CLK_GATE_SWITCH

extern const platform_uart_mapping_t uart_mapping[];

#endif


