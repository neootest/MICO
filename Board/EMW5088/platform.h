/**
******************************************************************************
* @file    platform.h
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provides all MICO Peripherals defined for current platform.
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

#include "platform_common_config.h"

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
  
#define HARDWARE_REVISION   "EMW5088_1"
#define DEFAULT_NAME        "EMW5088"
#define MODEL               "EMW5088"

   
/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
  MICO_GPIO_UNUSED = -1,
  WL_GPIO1,
  WL_REG,
  MICO_SYS_LED,
  MICO_RF_LED,
  BOOT_SEL,
  MFG_SEL,
  EasyLink_BUTTON,
  STDIO_UART_RX,
  STDIO_UART_TX,
  SDIO_INT,
  USB_DETECT,  
  APP_UART_RX,
  APP_UART_TX,
  MICO_GPIO_MAX, /* Denotes the total number of GPIO port aliases. Not a valid GPIO alias */
} mico_gpio_t;

typedef enum
{
    MICO_SPI_1,
    MICO_SPI_MAX, /* Denotes the total number of SPI port aliases. Not a valid SPI alias */
} mico_spi_t;

typedef enum
{
    MICO_I2C_1,
    MICO_I2C_MAX, /* Denotes the total number of I2C port aliases. Not a valid I2C alias */
} mico_i2c_t;

typedef enum
{
    MICO_PWM_1,
    MICO_PWM_2,
    MICO_PWM_3,
    MICO_PWM_MAX, /* Denotes the total number of PWM port aliases. Not a valid PWM alias */
} mico_pwm_t;

typedef enum
{
    MICO_ADC_1,
    MICO_ADC_2,
    MICO_ADC_3,
    MICO_ADC_MAX, /* Denotes the total number of ADC port aliases. Not a valid ADC alias */
} mico_adc_t;

typedef enum
{
    MICO_UART_1,
    MICO_UART_2,
    MICO_UART_MAX, /* Denotes the total number of UART port aliases. Not a valid UART alias */
} mico_uart_t;

typedef enum
{
  MICO_SPI_FLASH,
  MICO_INTERNAL_FLASH,
  MICO_FLASH_MAX,
} mico_flash_t;

#ifdef BOOTLOADER
#define STDIO_UART       MICO_UART_1
#define STDIO_UART_BAUDRATE (115200) 
#else
#define STDIO_UART       MICO_UART_1
#define STDIO_UART_BAUDRATE (115200) 
#endif

#define UART_FOR_APP     MICO_UART_2
#define MFG_TEST         MICO_UART_1
#define CLI_UART         MICO_UART_1

/* Map interrupt to MiCO driver */
#define AP80xx_FUART MICO_UART_1
#define AP80xx_BUART MICO_UART_2

/* Components connected to external I/Os*/
#define WL_RESET            (MICO_GPIO_UNUSED)
#define Standby_SEL         (MICO_GPIO_UNUSED)

/* I/O connection <-> Peripheral Connections */
#define MICO_I2C_CP         (MICO_I2C_1)

#define RestoreDefault_TimeOut          3000  /**< Restore default and start easylink after 
                                                   press down EasyLink button for 3 seconds. */

#ifdef __cplusplus
} /*extern "C" */
#endif

