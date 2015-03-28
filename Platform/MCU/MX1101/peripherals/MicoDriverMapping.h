/**
******************************************************************************
* @file    stm32f2xx_platform.h 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide all the headers of functions for stm32f2xx platform
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

#include "MicoRtos.h"

/******************************************************
 *                      Macros
 ******************************************************/
 #define		GPIOA					(0x00)
 #define		GPIOB					(0x0A)
 #define		GPIOC					(0x14)

 #define		FUART					(0x00)
 #define		BUART					(0x01)

/******************************************************
 *                    Constants
 ******************************************************/
 #define NUMBER_OF_UART_INTERFACES  2

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
  uint8_t                       port;
  uint8_t                       pin;
} platform_pin_mapping_t;

typedef struct
{
  uint32_t                      pin;
} platform_adc_mapping_t;

typedef struct
{
  uint32_t                      pin;
} platform_pwm_mapping_t;


/* DMA can be enabled by setting SPI_USE_DMA */
typedef struct
{
  uint32_t                      pin;
} platform_spi_mapping_t;

typedef void (* wakeup_irq_handler_t)(void *arg);

typedef struct
{
    uint8_t                          uart;
    const platform_pin_mapping_t*    pin_tx;
    const platform_pin_mapping_t*    pin_rx;
    const platform_pin_mapping_t*    pin_cts;
    const platform_pin_mapping_t*    pin_rts;
    // peripheral_clock_t            usart_peripheral_clock;
    // peripheral_clock_func_t       usart_peripheral_clock_func;
    // irq_vector_t                  usart_irq;
    // dma_registers_t*              tx_dma;
    // dma_stream_registers_t*       tx_dma_stream;
    // uint8_t                       tx_dma_stream_number;
    // dma_channel_t                 tx_dma_channel;
    // peripheral_clock_t            tx_dma_peripheral_clock;
    // peripheral_clock_func_t       tx_dma_peripheral_clock_func;
    // irq_vector_t                  tx_dma_irq;
    // dma_registers_t*              rx_dma;
    // dma_stream_registers_t*       rx_dma_stream;
    // uint8_t                       rx_dma_stream_number;
    // dma_channel_t                 rx_dma_channel;
    // peripheral_clock_t            rx_dma_peripheral_clock;
    // peripheral_clock_func_t       rx_dma_peripheral_clock_func;
    // irq_vector_t                  rx_dma_irq;
} platform_uart_mapping_t;

typedef struct
{
  uint32_t                      pin;
} platform_i2c_mapping_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

extern const platform_pin_mapping_t  gpio_mapping[];
extern const platform_adc_mapping_t  adc_mapping[];
extern const platform_pwm_mapping_t  pwm_mappings[];
extern const platform_spi_mapping_t  spi_mapping[];
extern const platform_uart_mapping_t uart_mapping[];
extern const platform_i2c_mapping_t  i2c_mapping[];


/******************************************************
 *               Function Declarations
 ******************************************************/
