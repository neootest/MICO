/***
 * File: stm32_platform.h
 * test Code
 * Create by JerryYu @ Jan 12nd, 2015
 * Version: 0.1
 * */
#ifndef __STM32_PLATFORM_H__
#define __STM32_PLATFORM_H__

#include "stm32f2xx.h"
#include "stm32f2xx_dma.h"
#include "stm32f2xx_adc.h"
#include "stm32f2xx_rcc.h"
#include "stm32f2xx_gpio.h"
#include "stm32f2xx_tim.h"
#include "stm32f2xx_rtc.h"
#include "stm32f2xx_pwr.h"

void init_clocks();
void init_memory();
void init_architecture();
void init_platform_bootloader();


#endif

