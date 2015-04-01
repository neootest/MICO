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
#include "stm32f2xx.h"
#include "platform.h"
#include "platform_config.h"
#include "wlan_platform_common.h"

/******************************************************
 *                      Macros
 ******************************************************/
#ifdef __GNUC__
#define TRIGGER_BREAKPOINT() __asm__("bkpt")
#elif defined ( __IAR_SYSTEMS_ICC__ )
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
OSStatus host_platform_deinit_wlan_powersave_clock( void );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

void host_platform_reset_wifi( bool reset_asserted )
{
 ( reset_asserted == true) ? platform_gpio_output_low( &wifi_control_pins[ EMW1062_PIN_RESET ] ) : platform_gpio_output_high( &wifi_control_pins[ EMW1062_PIN_RESET ] );
}

void host_platform_power_wifi( bool power_enabled )
{
#if  defined ( MICO_USE_WIFI_POWER_PIN_ACTIVE_HIGH )
    ( power_enabled == true ) ? platform_gpio_output_high( &wifi_control_pins[EMW1062_PIN_POWER] ) : platform_gpio_output_low ( &wifi_control_pins[EMW1062_PIN_POWER] );
#else
    ( power_enabled == true ) ? platform_gpio_output_low ( &wifi_control_pins[EMW1062_PIN_POWER] ) : platform_gpio_output_high( &wifi_control_pins[EMW1062_PIN_POWER] );
#endif

}

OSStatus host_platform_init( void )
{
    host_platform_deinit_wlan_powersave_clock( );

    platform_gpio_init( &wifi_control_pins[EMW1062_PIN_RESET], OUTPUT_PUSH_PULL );
    host_platform_reset_wifi( true );  /* Start wifi chip in reset */
    
    platform_gpio_init( &wifi_control_pins[EMW1062_PIN_POWER], OUTPUT_PUSH_PULL );
    host_platform_power_wifi( false ); /* Start wifi chip with regulators off */

    return kNoErr;
}

OSStatus host_platform_deinit( void )
{
    platform_gpio_init( &wifi_control_pins[EMW1062_PIN_RESET], OUTPUT_PUSH_PULL );
    host_platform_reset_wifi( true );  /* Start wifi chip in reset */
    
    platform_gpio_init( &wifi_control_pins[EMW1062_PIN_POWER], OUTPUT_PUSH_PULL );
    host_platform_power_wifi( false ); /* Start wifi chip with regulators off */

    host_platform_deinit_wlan_powersave_clock( );

    return kNoErr;
}

bool host_platform_is_in_interrupt_context( void )
{
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
}

OSStatus host_platform_init_wlan_powersave_clock( void )
{

#if defined ( MICO_USE_WIFI_32K_CLOCK_MCO )
    platform_gpio_set_alternate_function( wifi_control_pins[EMW1062_PIN_32K_CLK].port, wifi_control_pins[EMW1062_PIN_32K_CLK].pin_number, GPIO_OType_PP, GPIO_PuPd_NOPULL, GPIO_AF_MCO );

    /* enable LSE output on MCO1 */
    RCC_MCO1Config( RCC_MCO1Source_LSE, RCC_MCO1Div_1 );
    return kNoErr;
#else

    return platform_reset_wlan_powersave_clock( );

#endif
}

OSStatus host_platform_deinit_wlan_powersave_clock( void )
{
    /* Tie the pin to ground */
    platform_gpio_init( &wifi_control_pins[EMW1062_PIN_32K_CLK], OUTPUT_PUSH_PULL );
    platform_gpio_output_low( &wifi_control_pins[EMW1062_PIN_32K_CLK] );
    return kNoErr;
}
