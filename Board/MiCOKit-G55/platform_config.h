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

#ifndef __PLATFORM_CONFIG_H__
#define __PLATFORM_CONFIG_H__

#pragma once

/******************************************************
*                      Macros
******************************************************/

/******************************************************
*                    Constants
******************************************************/

#define HARDWARE_REVISION   "MKG55_1"
#define DEFAULT_NAME        "MiCOKit G55"
#define MODEL               "MiCOKit-G55"
#define Bootloader_REVISION "V 0.1"

/* MICO RTOS tick rate in Hz */
#define MICO_DEFAULT_TICK_RATE_HZ                   (1000) 

/************************************************************************
 * Uncomment to disable watchdog. For debugging only */
#define MICO_DISABLE_WATCHDOG

/************************************************************************
 * Uncomment to disable standard IO, i.e. printf(), etc. */
//#define MICO_DISABLE_STDIO

/************************************************************************
 * Uncomment to disable MCU powersave API functions */
#define MICO_DISABLE_MCU_POWERSAVE

/************************************************************************
 * Uncomment to enable MCU real time clock */
//#define MICO_ENABLE_MCU_RTC

/************************************************************************
 * Restore default and start easylink after press down EasyLink button for 3 seconds. */
#define RestoreDefault_TimeOut                      (3000)

/** \name Resonator definitions
 *  @{ */
#define BOARD_FREQ_SLCK_XTAL      (32768U)
#define BOARD_FREQ_SLCK_BYPASS    (32768U)
#define BOARD_FREQ_MAINCK_XTAL    0 /* Not Mounted */
#define BOARD_FREQ_MAINCK_BYPASS  0 /* Not Mounted */
#define BOARD_MCK                 CHIP_FREQ_CPU_MAX
/*TBD startup time needs to be adjusted according to measurements */
#define BOARD_OSC_STARTUP_US      15625


/* ===== System Clock (MCK) Source Options */
/* #define CONFIG_SYSCLK_SOURCE        SYSCLK_SRC_SLCK_RC */
/* #define CONFIG_SYSCLK_SOURCE        SYSCLK_SRC_SLCK_XTAL */
/* #define CONFIG_SYSCLK_SOURCE        SYSCLK_SRC_SLCK_BYPASS */
/* #define CONFIG_SYSCLK_SOURCE        SYSCLK_SRC_MAINCK_8M_RC */
/* #define CONFIG_SYSCLK_SOURCE        SYSCLK_SRC_MAINCK_16M_RC */
/* #define CONFIG_SYSCLK_SOURCE        SYSCLK_SRC_MAINCK_24M_RC */
/* #define CONFIG_SYSCLK_SOURCE        SYSCLK_SRC_MAINCK_XTAL */
/* #define CONFIG_SYSCLK_SOURCE        SYSCLK_SRC_MAINCK_BYPASS */
#define CONFIG_SYSCLK_SOURCE        SYSCLK_SRC_PLLACK

/* ===== System Clock (MCK) Prescaler Options (Fmck = Fsys / (SYSCLK_PRES)) */
#define CONFIG_SYSCLK_PRES          SYSCLK_PRES_1
/* #define CONFIG_SYSCLK_PRES          SYSCLK_PRES_2 */
/* #define CONFIG_SYSCLK_PRES          SYSCLK_PRES_4 */
/* #define CONFIG_SYSCLK_PRES          SYSCLK_PRES_8 */
/* #define CONFIG_SYSCLK_PRES          SYSCLK_PRES_16 */
/* #define CONFIG_SYSCLK_PRES          SYSCLK_PRES_32 */
/* #define CONFIG_SYSCLK_PRES          SYSCLK_PRES_64 */
/* #define CONFIG_SYSCLK_PRES          SYSCLK_PRES_3 */

// ===== PLL0 (A) Options   (Fpll = (Fclk * PLL_mul) / PLL_div)
// Use mul and div effective values here.
#define CONFIG_PLL0_SOURCE          PLL_SRC_SLCK_XTAL
#define CONFIG_PLL0_MUL             3662
#define CONFIG_PLL0_DIV             1

// ===== Target frequency (System clock)
// - External XTAL frequency: 32768Hz
// - System clock source: SLCK XTAL
// - System clock prescaler: 1 (divided by 1)
// - PLLA source: SLCK_XTAL
// - PLLA output: SLCK_XTAL * 3662 / 1
// - System clock: SLCK_XTAL * 3662 / 1 / 1 = 120MHz


/******************************************************
 *  EMW1062 Options
 ******************************************************/
/*  Wi-Fi chip module */
#define EMW1062

/*  GPIO pins are used to bootstrap Wi-Fi to SDIO or gSPI mode */
#define MICO_WIFI_USE_GPIO_FOR_BOOTSTRAP

/*  Wi-Fi GPIO0 pin is used for out-of-band interrupt */
#define MICO_WIFI_OOB_IRQ_GPIO_PIN  ( 0 )

 /*  Wi-Fi power pin is present */
//#define MICO_USE_WIFI_POWER_PIN

/*  Wi-Fi reset pin is present */
#define MICO_USE_WIFI_RESET_PIN

/*  Wi-Fi 32K pin is present */
//#define MICO_USE_WIFI_32K_PIN

/*  USE SDIO 1bit mode */
//#define SDIO_1_BIT

/* Wi-Fi power pin is active high */
//#define MICO_USE_WIFI_POWER_PIN_ACTIVE_HIGH


/* Memory map */
/* Note: For MICO flash erase driver limitation, each flash area should be 16 kbytes align !!!!!!!!!!!*/

#if 1
#define MICO_FLASH_FOR_APPLICATION  MICO_INTERNAL_FLASH
#define APPLICATION_START_ADDRESS   (uint32_t)0x00420000
#define APPLICATION_END_ADDRESS     (uint32_t)0x0047FFFF
#define APPLICATION_FLASH_SIZE      (APPLICATION_END_ADDRESS - APPLICATION_START_ADDRESS + 1) /* 384k bytes*/

#define MICO_FLASH_FOR_UPDATE       MICO_SPI_FLASH  /* Optional */
#define UPDATE_START_ADDRESS        (uint32_t)0x00050000 /* Optional */
#define UPDATE_END_ADDRESS          (uint32_t)0x000AFFFF /* Optional */
#define UPDATE_FLASH_SIZE           (UPDATE_END_ADDRESS - UPDATE_START_ADDRESS + 1) /* 384k bytes, optional*/

#define MICO_FLASH_FOR_BOOT         MICO_INTERNAL_FLASH
#define BOOT_START_ADDRESS          (uint32_t)0x00400000
#define BOOT_END_ADDRESS            (uint32_t)0x00407FFF
#define BOOT_FLASH_SIZE             (BOOT_END_ADDRESS - BOOT_START_ADDRESS + 1) /* 32k bytes*/

#define MICO_FLASH_FOR_DRIVER       MICO_SPI_FLASH
#define DRIVER_START_ADDRESS        (uint32_t)0x00002000
#define DRIVER_END_ADDRESS          (uint32_t)0x0004FFFF
#define DRIVER_FLASH_SIZE           (DRIVER_END_ADDRESS - DRIVER_START_ADDRESS + 1) /* 312k bytes*/

#define MICO_FLASH_FOR_PARA         MICO_SPI_FLASH
#define PARA_START_ADDRESS          (uint32_t)0x00000000
#define PARA_END_ADDRESS            (uint32_t)0x00000FFF
#define PARA_FLASH_SIZE             (PARA_END_ADDRESS - PARA_START_ADDRESS + 1)   /* 4k bytes*/

#define MICO_FLASH_FOR_EX_PARA      MICO_SPI_FLASH
#define EX_PARA_START_ADDRESS       (uint32_t)0x00001000
#define EX_PARA_END_ADDRESS         (uint32_t)0x00001FFF
#define EX_PARA_FLASH_SIZE          (EX_PARA_END_ADDRESS - EX_PARA_START_ADDRESS + 1)   /* 4k bytes*/
#else
#define MICO_FLASH_FOR_APPLICATION  MICO_INTERNAL_FLASH
#define APPLICATION_START_ADDRESS   (uint32_t)0x00400000
#define APPLICATION_END_ADDRESS     (uint32_t)0x0047FFFF
#define APPLICATION_FLASH_SIZE      (APPLICATION_END_ADDRESS - APPLICATION_START_ADDRESS + 1) /* 448k bytes*/

//#define MICO_FLASH_FOR_UPDATE       MICO_SPI_FLASH  /* Optional */
//#define UPDATE_START_ADDRESS        (uint32_t)0x00050000 /* Optional */
//#define UPDATE_END_ADDRESS          (uint32_t)0x000AFFFF /* Optional */
//#define UPDATE_FLASH_SIZE           (UPDATE_END_ADDRESS - UPDATE_START_ADDRESS + 1) /* 384k bytes, optional*/

#define MICO_FLASH_FOR_BOOT         MICO_INTERNAL_FLASH
#define BOOT_START_ADDRESS          (uint32_t)0x00400000
#define BOOT_END_ADDRESS            (uint32_t)0x00407FFF
#define BOOT_FLASH_SIZE             (BOOT_END_ADDRESS - BOOT_START_ADDRESS + 1) /* 32k bytes*/

//#define MICO_FLASH_FOR_DRIVER       MICO_SPI_FLASH
//#define DRIVER_START_ADDRESS        (uint32_t)0x00002000
//#define DRIVER_END_ADDRESS          (uint32_t)0x0004FFFF
//#define DRIVER_FLASH_SIZE           (DRIVER_END_ADDRESS - DRIVER_START_ADDRESS + 1) /* 312k bytes*/

#define MICO_FLASH_FOR_PARA         MICO_INTERNAL_FLASH
#define PARA_START_ADDRESS          (uint32_t)0x00470000
#define PARA_END_ADDRESS            (uint32_t)0x00473FFF
#define PARA_FLASH_SIZE             (PARA_END_ADDRESS - PARA_START_ADDRESS + 1)   /* 16k bytes*/

#define MICO_FLASH_FOR_EX_PARA      MICO_INTERNAL_FLASH
#define EX_PARA_START_ADDRESS       (uint32_t)0x00474000
#define EX_PARA_END_ADDRESS         (uint32_t)0x0047FFFF
#define EX_PARA_FLASH_SIZE          (EX_PARA_END_ADDRESS - EX_PARA_START_ADDRESS + 1)   /* 16k bytes*/
#endif

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

#endif // __PLATFORM_CONFIG_H__
