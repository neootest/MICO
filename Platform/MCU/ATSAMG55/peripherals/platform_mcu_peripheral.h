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

#include "samg55.h"

#include "MicoRtos.h"
#include "RingBufferUtils.h"
  
// From module: Common SAM compiler driver
#include <compiler.h>
#include <status_codes.h>

// From module: Delay routines
#include <delay.h>

// From module: EEFC - Enhanced Embedded Flash Controller
#include <efc.h>

// From module: FLEXCOM - Flexible Serial Communication Controller
#include <flexcom.h>
// From module: Generic board support
#include <board.h>

// From module: IOPORT - General purpose I/O service
#include <ioport.h>

#include <pio.h>

// From module: Interrupt management - SAM implementation
#include <interrupt.h>

// From module: PDC - Peripheral DMA Controller Example
#include <pdc.h>

// From module: PMC - Power Management Controller
#include <pmc.h>
#include <sleep.h>

// From module: Part identification macros
#include <parts.h>

// From module: SAM FPU driver
#include <fpu.h>

// From module: SPI - Serial Peripheral Interface
#include <spi.h>

// From module: SUPC - Supply Controller
#include <supc.h>

// From module: Sleep manager - SAM implementation
#include <sam/sleepmgr.h>
#include <sleepmgr.h>

// From module: System Clock Control - SAMG implementation
#include <sysclk.h>

// From module: USART - Serial interface - SAM implementation for devices with only USART
#include <serial.h>
#ifndef USE_OWN_SPI_DRV
#include <spi_master_vec.h>
#endif
// From module: USART - Univ. Syn Async Rec/Trans
#include <usart.h>
  
#include <rtc.h>

#include "wdt.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define CREATE_IOPORT_PIN(port, pin) ((port) * 32 + (pin))

#define PORTA       IOPORT_PIOA
#define PORTB       IOPORT_PIOB    
  
/**
 * \brief Set peripheral mode for IOPORT pins.
 * It will configure port mode and disable pin mode (but enable peripheral).
 * \param port IOPORT port to configure
 * \param masks IOPORT pin masks to configure
 * \param mode Mode masks to configure for the specified pin (\ref ioport_modes)
 */
#define ioport_set_port_peripheral_mode(port, masks, mode) \
	do {\
		ioport_set_port_mode(port, masks, mode);\
		ioport_disable_port(port, masks);\
	} while (0)

/**
 * \brief Set peripheral mode for one single IOPORT pin.
 * It will configure port mode and disable pin mode (but enable peripheral).
 * \param pin IOPORT pin to configure
 * \param mode Mode masks to configure for the specified pin (\ref ioport_modes)
 */
#define ioport_set_pin_peripheral_mode(pin, mode) \
	do {\
		ioport_set_pin_mode(pin, mode);\
		ioport_disable_pin(pin);\
	} while (0)

/******************************************************
 *                    Constants
 ******************************************************/

 /* GPIOA to I */
#define NUMBER_OF_GPIO_PORTS      (8)

#define NUMBER_OF_GPIO_IRQ_LINES  (7)

/* USART1 to 6 */
#define NUMBER_OF_UART_PORTS      (6)


/* Invalid UART port number */
#define INVALID_UART_PORT_NUMBER  (0xff)

 /* SPI1 to SPI3 */
#define NUMBER_OF_SPI_PORTS       (3)

/******************************************************
 *                   Enumerations
 ******************************************************/
 /**
 * SPI slave transfer direction
 */
typedef enum
{
    FLASH_TYPE_INTERNAL, 
    FLASH_TYPE_SPI,   
} platform_flash_type_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/



/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    ioport_pin_t    pin;
    bool            is_wakeup_pin;
    uint8_t         wakeup_pin_number; /* wakeup pin number: 0 .. 15                     */
    uint8_t         trigger;           /* wakeup trigger: IOPORT_SENSE_FALLING or RISING */

} platform_gpio_t;

// typedef struct {
//     Usart                         *usart;
//     ioport_mode_t                 mux_mode;
//     ioport_port_t                 gpio_bank;
//     ioport_port_mask_t            pin_tx;
//     ioport_port_mask_t            pin_rx;
//     ioport_port_mask_t            pin_cts;
//     ioport_port_mask_t            pin_rts;
//     Flexcom                       *flexcom_base;
//     uint32_t                      id_peripheral_clock;
//     IRQn_Type                     usart_irq;
//     Pdc                           *dma_base;
// } platform_uart_t;

typedef struct {
    uint8_t                uart_id;
    void*                  peripheral;       /* Usart* or Uart*  */
    uint8_t                peripheral_id;    /* Peripheral ID    */
    const platform_gpio_t* tx_pin;           /* Tx pin           */
    ioport_mode_t          tx_pin_mux_mode;  /* Tx pin mux mode  */
    const platform_gpio_t* rx_pin;           /* Rx pin           */
    ioport_mode_t          rx_pin_mux_mode;  /* Tx pin mux mode  */
    const platform_gpio_t* cts_pin;          /* CTS pin          */
    ioport_mode_t          cts_pin_mux_mode; /* Tx pin mux mode  */
    const platform_gpio_t* rts_pin;          /* RTS pin          */
    ioport_mode_t          rts_pin_mux_mode; /* Tx pin mux mode  */
} platform_uart_t;


typedef struct
{
    platform_uart_t*           peripheral;
    ring_buffer_t*             rx_ring_buffer;
#ifndef NO_MICO_RTOS
    mico_semaphore_t           rx_complete;
    mico_semaphore_t           tx_complete;
    mico_mutex_t               tx_mutex;
    mico_semaphore_t           sem_wakeup;
#else
    volatile bool              rx_complete;
    volatile bool              tx_complete;
#endif
    volatile uint32_t          tx_size;
    volatile uint32_t          rx_size;
    volatile OSStatus          last_receive_result;
    volatile OSStatus          last_transmit_result;
} platform_uart_driver_t;

typedef struct
{
    platform_flash_type_t      flash_type;
    uint32_t                   flash_start_addr;
    uint32_t                   flash_length;
} platform_flash_t;

typedef struct
{
    platform_flash_t*          peripheral;
    bool                       initialized;
#ifndef NO_MICO_RTOS
    mico_mutex_t               flash_mutex;
#endif
} platform_flash_driver_t;

typedef struct
{
    uint8_t unimplemented;
} platform_spi_t;

typedef struct
{
    uint8_t unimplemented;
} platform_spi_slave_driver_t;

typedef struct
{
    uint8_t unimplemented;
} platform_adc_t;

typedef struct
{
    uint8_t unimplemented;
} platform_pwm_t;

typedef struct
{
    uint8_t unimplemented;
} platform_i2c_t;

/******************************************************
 *                 Global Variables
 ******************************************************/


/******************************************************
 *               Function Declarations
 ******************************************************/
OSStatus platform_gpio_irq_manager_init      ( void );

OSStatus platform_mcu_powersave_init         ( void );

OSStatus platform_rtc_init                   ( void );

OSStatus platform_gpio_peripheral_pin_init( const platform_gpio_t* gpio, ioport_mode_t pin_mode );

void     platform_uart_irq                   ( platform_uart_driver_t* driver );
void     platform_uart_tx_dma_irq            ( platform_uart_driver_t* driver );
void     platform_uart_rx_dma_irq            ( platform_uart_driver_t* driver );


#ifdef __cplusplus
} /* extern "C" */
#endif





