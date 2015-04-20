/**
******************************************************************************
* @file    mico_common_driver.c 
* @author  jerry yu
* @version V1.0.0
* @date    Mar 11st 2015
* @brief   mico common driver typedef.
******************************************************************************
*
*  the MIT license
*  copyright (c) 20155555 MXCHIP inc.
*
*  permission is hereby granted, free of charge, to any person obtaining a copy 
*  of this software and associated documentation files (the "software"), to deal
*  in the software without restriction, including without limitation the rights 
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the software, and to permit persons to whom the software is furnished
*  to do so, subject to the following conditions:
*
*  the above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
*  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************
*/ 
#ifndef __MICO_COMMON_DRIVER_H__
#define __MICO_COMMON_DRIVER_H__

#include "samg55.h"

#include "mico_rtos.h"

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

// From module: SAMG55 Xplained Pro LED support enabled
#include <led.h>

// From module: SPI - Serial Peripheral Interface
#include <spi.h>

// From module: SUPC - Supply Controller
#include <supc.h>

// From module: Sleep manager - SAM implementation
#include <sam/sleepmgr.h>
#include <sleepmgr.h>

// From module: Standard serial I/O (stdio) - SAM implementation
#include <stdio_serial.h>

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

#define CREATE_IOPORT_PIN(port, pin) ((port) * 32 + (pin))

#define PORTA       IOPORT_PIOA
#define PORTB       IOPORT_PIOB
#if 1 //will enable it
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
#endif

typedef struct {
    ioport_port_t   bank;
    uint8_t         number;
} platform_pin_mapping_t;

typedef struct {
    Usart                         *usart;
    ioport_mode_t                 mux_mode;
    ioport_port_t                 gpio_bank;
    ioport_port_mask_t            pin_tx;
    ioport_port_mask_t            pin_rx;
    ioport_port_mask_t            pin_cts;
    ioport_port_mask_t            pin_rts;
    Flexcom                       *flexcom_base;
    uint32_t                      id_peripheral_clock;
    IRQn_Type                     usart_irq;
    Pdc                           *dma_base;
} platform_uart_mapping_t;

extern const platform_pin_mapping_t  gpio_mapping[];
//extern const platform_adc_mapping_t  adc_mapping[];
//extern const platform_pwm_mapping_t  pwm_mappings[];
//extern const platform_spi_mapping_t  spi_mapping[];
extern const platform_uart_mapping_t uart_mapping[];
//extern const platform_i2c_mapping_t  i2c_mapping[];
#endif 

