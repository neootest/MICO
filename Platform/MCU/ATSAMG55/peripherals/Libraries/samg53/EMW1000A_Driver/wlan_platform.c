/**
******************************************************************************
* @file    wlan_platform.c 
* @author  william xu
* @version V1.0.0
* @date    05-may-2014
* @brief   this file provide functions called by MICO to wlan RF module
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

#include <stdint.h>
#include "gpio_irq.h"
#include "platform.h"
#include "platform.h"
#include "mico_platform.h"

/******************************************************
 *                      macros
 ******************************************************/
#ifdef __GNUC__
#define TRIGGER_BREAKPOINT() __asm__("bkpt")
#elif defined ( __IAR_SYSTEMS_ICC__ )
#define TRIGGER_BREAKPOINT() __asm("bkpt 0")
#endif

/******************************************************
 *                    constants
 ******************************************************/

/******************************************************
 *                   enumerations
 ******************************************************/

/******************************************************
 *                 type definitions
 ******************************************************/

/******************************************************
 *                    structures
 ******************************************************/

/******************************************************
 *               function declarations
 ******************************************************/

static OSStatus platform_reset_wlan_powersave_clock( void );

extern void host_platform_reset_wifi( bool reset_asserted );

extern void host_platform_power_wifi( bool power_enabled );

/******************************************************
 *               variables definitions
 ******************************************************/

/******************************************************
 *               function definitions
 ******************************************************/

OSStatus host_platform_init( void )
{
    platform_reset_wlan_powersave_clock( );
    
    mico_gpio_initialize((mico_gpio_t)WL_RESET, OUTPUT_PUSH_PULL);
    host_platform_reset_wifi( true ); /* start wifi chip in reset */
    
    //mico_gpio_initialize((mico_gpio_t)WL_REG, OUTPUT_PUSH_PULL);
    host_platform_power_wifi( false ); /* start wifi chip with regulators off */

    return kNoErr;
}

OSStatus host_platform_deinit( void )
{
    mico_gpio_initialize((mico_gpio_t)WL_RESET, OUTPUT_PUSH_PULL);
    host_platform_reset_wifi( true ); /* stop wifi chip in reset */
    
    //mico_gpio_initialize((mico_gpio_t)WL_REG, OUTPUT_PUSH_PULL);
    host_platform_power_wifi( false ); /* stop wifi chip with regulators off */

    platform_reset_wlan_powersave_clock( );

    return kNoErr;
}

bool host_platform_is_in_interrupt_context( void )
{
    /* from the ARM cortex-M3 techinical reference manual
     * 0xE000ED04   ICSR    RW [a]  privileged  0x00000000  interrupt control and state register */
    uint32_t active_interrupt_vector = (uint32_t)( SCB ->ICSR & 0x3fU );

    if ( active_interrupt_vector != 0 )
    {
        return true;
    }
    else
    {
        return false;
    }
}


OSStatus host_platform_init_wlan_powersave_clock( void )
{
#if ( MICO_WLAN_POWERSAVE_CLOCK_SOURCE == MICO_WLAN_POWERSAVE_CLOCK_IS_PWM )

    return kNoErr; //WICED_SUCCESS;

#elif ( MICO_WLAN_POWERSAVE_CLOCK_SOURCE == MICO_WLAN_POWERSAVE_CLOCK_IS_MCO )


    /* Enable the GPIO peripherals related to the 32kHz clock pin */

    return kNoErr;

#else

    return platform_reset_wlan_powersave_clock( );

#endif
}

OSStatus host_platform_deinit_wlan_powersave_clock( void )
{
#if ( MICO_WLAN_POWERSAVE_CLOCK_SOURCE == MICO_WLAN_POWERSAVE_CLOCK_IS_PWM )

    platform_reset_wlan_powersave_clock( );
    return kNoErr;

#else

    return platform_reset_wlan_powersave_clock( );

#endif
}

static OSStatus platform_reset_wlan_powersave_clock( void )
{
    /* tie the pin to ground */
    return kNoErr;
}
