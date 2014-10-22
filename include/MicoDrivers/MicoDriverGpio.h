/**
******************************************************************************
* @file    MicoDriverGpio.h 
* @author  William Xu
* @version V1.0.0
* @date    16-Sep-2014
* @brief   This file provides all the headers of GPIO operation functions.
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

#ifndef __MICODRIVERGPIO_H__
#define __MICODRIVERGPIO_H__

#pragma once
#include "Common.h"
#include "platform.h"

/** @addtogroup MICO_PLATFORM
* @{
*/

/******************************************************
 *                   Macros
 ******************************************************/  

#define MicoGpioInputGet wiced_gpio_input_get

/******************************************************
 *                   Enumerations
 ******************************************************/

 typedef enum
{
    INPUT_PULL_UP,                  /**< Input with an internal pull-up resistor - use with devices that actively drive the signal low - e.g. button connected to ground */
    INPUT_PULL_DOWN,                /**< Input with an internal pull-down resistor - use with devices that actively drive the signal high - e.g. button connected to a power rail */
    INPUT_HIGH_IMPEDANCE,           /**< Input - must always be driven, either actively or by an external pullup resistor */
    OUTPUT_PUSH_PULL,               /**< Output actively driven high and actively driven low - must not be connected to other active outputs - e.g. LED output */
    OUTPUT_OPEN_DRAIN_NO_PULL,      /**< Output actively driven low but is high-impedance when set high - can be connected to other open-drain/open-collector outputs. Needs an external pull-up resistor */
    OUTPUT_OPEN_DRAIN_PULL_UP,      /**< Output actively driven low and is pulled high with an internal resistor when set high - can be connected to other open-drain/open-collector outputs. */
} mico_gpio_config_t;

typedef enum
{
    IRQ_TRIGGER_RISING_EDGE  = 0x1, /**< Interrupt triggered at input signal's rising edge  */
    IRQ_TRIGGER_FALLING_EDGE = 0x2, /**< Interrupt triggered at input signal's falling edge */
    IRQ_TRIGGER_BOTH_EDGES   = IRQ_TRIGGER_RISING_EDGE | IRQ_TRIGGER_FALLING_EDGE, /**< Interrupt triggered at input signal's rising or falling edge */
} mico_gpio_irq_trigger_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

 /******************************************************
 *                 Function Declarations
 ******************************************************/

typedef void (*mico_gpio_irq_handler_t)( void* arg );

/** @defgroup MICO_GPIO MICO GPIO Driver
* @brief  General Purpose Input/Output pin (GPIO) Functions
* @{
*/

/** Initialises a GPIO pin
 *
 * Prepares a GPIO pin for use.
 *
 * @param gpio          : the gpio pin which should be initialised
 * @param configuration : A structure containing the required
 *                        gpio configuration
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred with any step
 */
OSStatus MicoGpioInitialize( mico_gpio_t gpio, mico_gpio_config_t configuration );


/** DeInitialises a GPIO pin
 *
 * Set a GPIO pin in default state.
 *
 * @param gpio          : the gpio pin which should be deinitialised
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred with any step
 */
OSStatus MicoGpioFinalize( mico_gpio_t gpio );


/** Sets an output GPIO pin high
 *
 * Sets an output GPIO pin high. Using this function on a
 * gpio pin which is set to input mode is undefined.
 *
 * @param gpio          : the gpio pin which should be set high
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred with any step
 */
OSStatus MicoGpioOutputHigh( mico_gpio_t gpio );


/** Sets an output GPIO pin low
 *
 * Sets an output GPIO pin low. Using this function on a
 * gpio pin which is set to input mode is undefined.
 *
 * @param gpio          : the gpio pin which should be set low
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred with any step
 */
OSStatus MicoGpioOutputLow( mico_gpio_t gpio );

/** Trigger an output GPIO pin 
 *
 * Trigger an output GPIO pin's output. Using this function on a
 * gpio pin which is set to input mode is undefined.
 *
 * @param gpio          : the gpio pin which should be set low
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred with any step
 */
OSStatus MicoGpioOutputTrigger( mico_gpio_t gpio );



/** Get the state of an input GPIO pin
 *
 * Get the state of an input GPIO pin. Using this function on a
 * gpio pin which is set to output mode will return an undefined value.
 *
 * @param gpio          : the gpio pin which should be read
 *
 * @return    true  : if high
 * @return    fasle : if low
 */
bool MicoGpioInputGet( mico_gpio_t gpio );


/** Enables an interrupt trigger for an input GPIO pin
 *
 * Enables an interrupt trigger for an input GPIO pin.
 * Using this function on a gpio pin which is set to
 * output mode is undefined.
 *
 * @param gpio    : the gpio pin which will provide the interrupt trigger
 * @param trigger : the type of trigger (rising/falling edge)
 * @param handler : a function pointer to the interrupt handler
 * @param arg     : an argument that will be passed to the
 *                  interrupt handler
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred with any step
 */
OSStatus MicoGpioEnableIRQ( mico_gpio_t gpio, mico_gpio_irq_trigger_t trigger, mico_gpio_irq_handler_t handler, void* arg );


/** Disables an interrupt trigger for an input GPIO pin
 *
 * Disables an interrupt trigger for an input GPIO pin.
 * Using this function on a gpio pin which has not been set up
 * using @ref wiced_gpio_input_irq_enable is undefined.
 *
 * @param gpio    : the gpio pin which provided the interrupt trigger
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred with any step
 */
OSStatus MicoGpioDisableIRQ( mico_gpio_t gpio );

/** @} */
/** @} */

#endif


