/**
******************************************************************************
* @file    gpio_irq.h 
* @author  william xu
* @version V1.0.0
* @date    05-may-2014
* @brief   this file provides all the headers of GPIO external interrupt 
*          operation functions.
******************************************************************************
*
*  the MIT license
*  copyright (c) 2014 MXCHIP inc.
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

#pragma once

#include <stdint.h>
#include "mico_common_driver.h"
#include "mico_platform.h"

/******************************************************
 *                      macros
 ******************************************************/
#define IRQ_PRIORITY_PIO    0
#define ioport_port_to_base(port) (IOPORT_BASE_ADDRESS + (IOPORT_PIO_OFFSET * port))
/******************************************************
 *                    constants
 ******************************************************/

/******************************************************
 *                   enumerations
 ******************************************************/

/******************************************************
 *                 type definitions
 ******************************************************/

typedef ioport_port_t           gpio_port_t;
typedef uint8_t                 gpio_pin_number_t;
typedef mico_gpio_irq_trigger_t gpio_irq_trigger_t;
typedef mico_gpio_irq_handler_t gpio_irq_handler_t;

/******************************************************
 *                    structures
 ******************************************************/

/******************************************************
 *                 global variables
 ******************************************************/

/******************************************************
 *               function declarations
 ******************************************************/
/*__always_inline Pio *iport_port_to_base( ioport_port_t port) 
{
	return (Pio *)((uintptr_t)IOPORT_BASE_ADDRESS +
	       (IOPORT_PIO_OFFSET * port));
}*/

//void pio_handler_set_priority(Pio *p_pio, IRQn_Type ul_irqn, uint32_t ul_priority);
/* global GPIO interrupt handler */
void gpio_irq(Pio *p_pio, uint32_t ul_id);

/* GPIO interrupt API */
OSStatus gpio_irq_enable ( gpio_port_t  gpio_port, gpio_pin_number_t gpio_pin_number, gpio_irq_trigger_t trigger, gpio_irq_handler_t handler, void *arg );
OSStatus gpio_irq_disable( gpio_port_t  gpio_port, gpio_pin_number_t gpio_pin_number );
