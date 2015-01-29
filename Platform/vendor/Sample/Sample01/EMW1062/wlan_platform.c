/**
******************************************************************************
* @file    wlan_platform.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide functions called by MICO to wlan RF module
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

#include <stdint.h>
#include "gpio_irq.h"
#include "platform.h"
#include "platform_common_config.h"
#include "MICOPlatform.h"

/******************************************************
 *                      Macros
 ******************************************************/
#ifdef __GNUC__
#define TRIGGER_BREAKPOINT() __asm__("bkpt")
#elif defined ( __IAR_SYSTEMS_ICC__ )
#define TRIGGER_BREAKPOINT() __asm("bkpt 0")
#elif defined ( __CC_ARM )
#define TRIGGER_BREAKPOINT() __asm("bkpt 0")
#endif

/******************************************************
 *                    Constants
 ******************************************************/

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
 *               Function Declarations
 ******************************************************/

static OSStatus platform_reset_wlan_powersave_clock( void );

extern void host_platform_reset_wifi( bool reset_asserted );

extern void host_platform_power_wifi( bool power_enabled );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

OSStatus host_platform_init( void )
{
#if 0
    platform_reset_wlan_powersave_clock( );
    
    MicoGpioInitialize((mico_gpio_t)WL_RESET, OUTPUT_PUSH_PULL);
    host_platform_reset_wifi( true ); /* Start wifi chip in reset */
    
    MicoGpioInitialize((mico_gpio_t)WL_REG, OUTPUT_PUSH_PULL);
    host_platform_power_wifi( false ); /* Start wifi chip with regulators off */
#endif
    return kNoErr;
}

OSStatus host_platform_deinit( void )
{
#if 0
    MicoGpioInitialize((mico_gpio_t)WL_RESET, OUTPUT_PUSH_PULL);
    host_platform_reset_wifi( true ); /* Stop wifi chip in reset */
    
    MicoGpioInitialize((mico_gpio_t)WL_REG, OUTPUT_PUSH_PULL);
    host_platform_power_wifi( false ); /* Stop wifi chip with regulators off */

    platform_reset_wlan_powersave_clock( );
#endif
    return kNoErr;
}

bool host_platform_is_in_interrupt_context( void )
{
#if 0
    /* From the ARM Cortex-M3 Techinical Reference Manual
     * 0xE000ED04   ICSR    RW [a]  Privileged  0x00000000  Interrupt Control and State Register */
    uint32_t active_interrupt_vector = (uint32_t)( SCB ->ICSR & 0x3fU );

    if ( active_interrupt_vector != 0 )
    {
        return true;
    }
    else
    {
        return false;
    }
#else
    return false;
#endif 
}


OSStatus host_platform_init_wlan_powersave_clock( void )
{

    return platform_reset_wlan_powersave_clock( );

}

OSStatus host_platform_deinit_wlan_powersave_clock( void )
{
    return platform_reset_wlan_powersave_clock( );

}

static OSStatus platform_reset_wlan_powersave_clock( void )
{   
//  /* Tie the pin to ground */
//  MicoGpioInitialize( (mico_gpio_t) MICO_GPIO_WLAN_POWERSAVE_CLOCK, OUTPUT_PUSH_PULL );
//  MicoGpioOutputLow( (mico_gpio_t) MICO_GPIO_WLAN_POWERSAVE_CLOCK );
  return kNoErr;
}
