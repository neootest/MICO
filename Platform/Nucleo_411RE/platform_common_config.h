/**
******************************************************************************
* @file    platform_common_config.h
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provides common configuration for current platform.
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

#pragma once

/******************************************************
*                      Macros
******************************************************/

/******************************************************
*                    Constants
******************************************************/

/* MICO RTOS tick rate in Hz */
#define MICO_DEFAULT_TICK_RATE_HZ                   (1000) 

/************************************************************************
 * Uncomment to disable watchdog. For debugging only */
//#define MICO_DISABLE_WATCHDOG

/************************************************************************
 * Uncomment to disable standard IO, i.e. printf(), etc. */
//#define MICO_DISABLE_STDIO

/************************************************************************
 * Uncomment to disable MCU powersave API functions */
//#define MICO_DISABLE_MCU_POWERSAVE

/************************************************************************
 * Uncomment to enable MCU real time clock */
#define MICO_ENABLE_MCU_RTC


#define MCU_CLOCK_HZ            100000000

#define HSE_SOURCE              RCC_HSE_OFF               /* Use external crystal                 */
#define AHB_CLOCK_DIVIDER       RCC_SYSCLK_Div1          /* AHB clock = System clock             */
#define APB1_CLOCK_DIVIDER      RCC_HCLK_Div2            /* APB1 clock = AHB clock / 2           */
#define APB2_CLOCK_DIVIDER      RCC_HCLK_Div1            /* APB2 clock = AHB clock / 1           */
#define PLL_SOURCE              RCC_PLLSource_HSI        /* PLL source = external crystal        */
#define PLL_M_CONSTANT          16                        /* PLLM = 16                            */
#define PLL_N_CONSTANT          400                      /* PLLN = 400                           */
#define PLL_P_CONSTANT          4                        /* PLLP = 4                             */
#define PPL_Q_CONSTANT          7                        /* PLLQ = 7                             */
#define SYSTEM_CLOCK_SOURCE     RCC_SYSCLKSource_PLLCLK  /* System clock source = PLL clock      */
#define SYSTICK_CLOCK_SOURCE    SysTick_CLKSource_HCLK   /* SysTick clock source = AHB clock     */
#define INT_FLASH_WAIT_STATE    FLASH_Latency_3          /* Internal flash wait state = 3 cycles */

/************************************************************************
 * Used for EMW1062 RF SPI driver */
#define SPI_BUS_CLOCK_BANK      GPIOA
#define SPI_BUS_MISO_BANK       GPIOA
#define SPI_BUS_MOSI_BANK       GPIOA
#define SPI_BUS_CS_BANK         GPIOB
#define SPI_IRQ_BANK            GPIOC
#define SPI_BUS_CLOCK_BANK_CLK  RCC_AHB1Periph_GPIOA
#define SPI_BUS_MISO_BANK_CLK   RCC_AHB1Periph_GPIOA
#define SPI_BUS_MOSI_BANK_CLK   RCC_AHB1Periph_GPIOA
#define SPI_BUS_CS_BANK_CLK     RCC_AHB1Periph_GPIOB
#define SPI_IRQ_CLK             RCC_AHB1Periph_GPIOC
#define SPI_BUS_CLOCK_PIN       5
#define SPI_BUS_MISO_PIN        6
#define SPI_BUS_MOSI_PIN        7
#define SPI_BUS_CS_PIN          6
#define SPI_IRQ_PIN             7

#define SPIX_CLK_FUNCTION       RCC_APB2PeriphClockCmd
#define SPIX_CLK                RCC_APB2Periph_SPI1  
#define SPIX_DMA_CLK            RCC_AHB1Periph_DMA2  
#define SPIX                    SPI1                
#define SPIX_AF                 GPIO_AF_SPI1
#define SPIX_DMA_RX_STREAM      DMA2_Stream0   
#define SPIX_DMA_TX_STREAM      DMA2_Stream3     
#define SPIX_DMA_RX_TCFLAG      DMA_FLAG_TCIF0
#define SPIX_DMA_TX_TCFLAG      DMA_FLAG_TCIF3
#define SPIX_DMA_RX_CHANNEL     DMA_Channel_3
#define SPIX_DMA_TX_CHANNEL     DMA_Channel_3
#define SPIX_DMA_RX_IRQ_CHANNEL DMA2_Stream0_IRQn
#define SPIX_DMA_RX_TCIT        DMA_IT_TCIF0
#define dma_irq                 DMA2_Stream0_IRQHandler

/* These are internal platform connections only */
typedef enum
{
  MICO_GPIO_UNUSED = -1,
  MICO_GPIO_WLAN_POWERSAVE_CLOCK = 0,
  WL_GPIO0,
  WL_GPIO1,
  WL_RESET,
  MICO_SYS_LED,
  MICO_RF_LED,
  BOOT_SEL,
  MFG_SEL,
  Standby_SEL,
  EasyLink_BUTTON,
  STDIO_UART_RX,  
  STDIO_UART_TX,  
  MICO_COMMON_GPIO_MAX,
} mico_common_gpio_t;

#define WL_REG MICO_GPIO_UNUSED

/* How the wlan's powersave clock is connected */
typedef enum
{
  MICO_PWM_WLAN_POWERSAVE_CLOCK,
  MICO_COMMON_PWM_MAX,
} mico_common_pwm_t;

/* WLAN Powersave Clock Source
 * The WLAN sleep clock can be driven from one of two sources:
 * 1. Timer/PWM (default)
 *    - With the PWM selected, the STM32 can *NOT* be put into MCU powersave mode or the PWM output will be disabled
 * 2. MCO (MCU Clock Output). 
 *    - Change the following directive to MICO_WLAN_POWERSAVE_CLOCK_IS_MCO
 */
#define MICO_WLAN_POWERSAVE_CLOCK_SOURCE MICO_WLAN_POWERSAVE_CLOCK_IS_PWM

#define MICO_WLAN_POWERSAVE_CLOCK_IS_NOT_EXIST  0
#define MICO_WLAN_POWERSAVE_CLOCK_IS_PWM        1
#define MICO_WLAN_POWERSAVE_CLOCK_IS_MCO        2


#define WLAN_POWERSAVE_CLOCK_FREQUENCY 32768 /* 32768Hz        */
#define WLAN_POWERSAVE_CLOCK_DUTY_CYCLE   50 /* 50% duty-cycle */

#define WL_32K_OUT_BANK         GPIOA
#define WL_32K_OUT_PIN          8
#define WL_32K_OUT_BANK_CLK     RCC_AHB1Periph_GPIOA

/* The number of UART interfaces this hardware platform has */
#define NUMBER_OF_UART_INTERFACES  2

#define UART_FOR_APP     MICO_UART_1
#define STDIO_UART       MICO_UART_1
#define MFG_TEST         MICO_UART_1
#define CLI_UART         MICO_UART_2

/* Memory map */
#define INTERNAL_FLASH_START_ADDRESS    (uint32_t)0x08000000
#define INTERNAL_FLASH_END_ADDRESS      (uint32_t)0x0807FFFF
#define INTERNAL_FLASH_SIZE             (INTERNAL_FLASH_END_ADDRESS - INTERNAL_FLASH_START_ADDRESS + 1) /* 512k bytes*/

#define SPI_FLASH_START_ADDRESS         (uint32_t)0x00000000
#define SPI_FLASH_END_ADDRESS           (uint32_t)0x000FFFFF
#define SPI_FLASH_SIZE                  (SPI_FLASH_END_ADDRESS - SPI_FLASH_START_ADDRESS + 1) /* 1M bytes*/

#define MICO_FLASH_FOR_APPLICATION  MICO_INTERNAL_FLASH
#define APPLICATION_START_ADDRESS   (uint32_t)0x0800C000
#define APPLICATION_END_ADDRESS     (uint32_t)0x0807FFFF
#define APPLICATION_FLASH_SIZE      (APPLICATION_END_ADDRESS - APPLICATION_START_ADDRESS + 1) /* 480k bytes*/

#define MICO_FLASH_FOR_UPDATE       MICO_SPI_FLASH  
#define UPDATE_START_ADDRESS        (uint32_t)0x00040000 
#define UPDATE_END_ADDRESS          (uint32_t)0x0009FFFF 
#define UPDATE_FLASH_SIZE           (UPDATE_END_ADDRESS - UPDATE_START_ADDRESS + 1) /* 256k bytes, optional*/

#define MICO_FLASH_FOR_BOOT         MICO_INTERNAL_FLASH
#define BOOT_START_ADDRESS          (uint32_t)0x08000000
#define BOOT_END_ADDRESS            (uint32_t)0x08007FFF
#define BOOT_FLASH_SIZE             (BOOT_END_ADDRESS - BOOT_START_ADDRESS + 1) /* 32k bytes*/

#define MICO_FLASH_FOR_DRIVER       MICO_SPI_FLASH
#define DRIVER_START_ADDRESS        (uint32_t)0x00002000
#define DRIVER_END_ADDRESS          (uint32_t)0x0003FFFF
#define DRIVER_FLASH_SIZE           (DRIVER_END_ADDRESS - DRIVER_START_ADDRESS + 1) /* 248k bytes*/

#define MICO_FLASH_FOR_PARA         MICO_SPI_FLASH
#define PARA_START_ADDRESS          (uint32_t)0x00000000
#define PARA_END_ADDRESS            (uint32_t)0x00000FFF
#define PARA_FLASH_SIZE             (PARA_END_ADDRESS - PARA_START_ADDRESS + 1)   /* 4k bytes*/

#define MICO_FLASH_FOR_EX_PARA      MICO_SPI_FLASH
#define EX_PARA_START_ADDRESS       (uint32_t)0x00001000
#define EX_PARA_END_ADDRESS         (uint32_t)0x00001FFF
#define EX_PARA_FLASH_SIZE          (EX_PARA_END_ADDRESS - EX_PARA_START_ADDRESS + 1)   /* 4k bytes*/

/******************************************************
*                   Enumerations
******************************************************/

/******************************************************
*                 Type Definitions
******************************************************/

/******************************************************
*                    Structures
******************************************************/

/******************************************************
*                 Global Variables
******************************************************/

/******************************************************
*               Function Declarations
******************************************************/
