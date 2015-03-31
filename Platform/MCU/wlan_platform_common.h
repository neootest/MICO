/**
******************************************************************************
* @file    wlan_platform_common.h
* @author  William Xu
* @version V1.0.0
* @date    20-March-2015
* @brief   This file provide wlan IO pin define.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2015 MXCHIP Inc.
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

#include "platform_peripheral.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/**
 * WLAN control pins
 */
typedef enum
{
    EMW1062_PIN_POWER,
    EMW1062_PIN_RESET,
    EMW1062_PIN_32K_CLK,
    EMW1062_PIN_BOOTSTRAP_0,
    EMW1062_PIN_BOOTSTRAP_1,
    EMW1062_PIN_CONTROL_MAX,
} emw1062_control_pin_t;

/**
 * WLAN SDIO pins
 */
typedef enum
{
    EMW1062_PIN_SDIO_OOB_IRQ,
    EMW1062_PIN_SDIO_CLK,
    EMW1062_PIN_SDIO_CMD,
    EMW1062_PIN_SDIO_D0,
    EMW1062_PIN_SDIO_D1,
    EMW1062_PIN_SDIO_D2,
    EMW1062_PIN_SDIO_D3,
    EMW1062_PIN_SDIO_MAX,
} emw1062_sdio_pin_t;

/**
 * WLAN SPI pins
 */
typedef enum
{
    EMW1062_PIN_SPI_IRQ,
    EMW1062_PIN_SPI_CS,
    EMW1062_PIN_SPI_CLK,
    EMW1062_PIN_SPI_MOSI,
    EMW1062_PIN_SPI_MISO,
    EMW1062_PIN_SPI_MAX,
} emw1062_spi_pin_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/* Externed from <WICED-SDK>/platforms/<Platform>/platform.c */
extern const platform_gpio_t wifi_control_pins[];
extern const platform_gpio_t wifi_sdio_pins   [];
extern const platform_gpio_t wifi_spi_pins    [];

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif
