/**
******************************************************************************
* @file    platform.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provides all MICO Peripherals mapping table and platform
*          specific funcgtions.
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


#include "MICOPlatform.h"
#include "platform_wifi_config.h"
#include "platform_mico.h"
#include "PlatformLogging.h"

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

/******************************************************
*                    Structures
******************************************************/

/******************************************************
*               Function Declarations
******************************************************/
extern WEAK void PlatformEasyLinkButtonClickedCallback(void);
extern WEAK void PlatformStandbyButtonClickedCallback(void);
extern WEAK void PlatformEasyLinkButtonLongPressedCallback(void);
extern WEAK void bootloader_start(void);

static uint32_t _default_start_time = 0;
static mico_timer_t _button_EL_timer;

OSStatus mico_platform_init( void )
{
  platform_log( "Platform initialised" );
  
  return kNoErr;
}

void init_platform( void )
{
}

void init_platform_bootloader( void )
{
  
}
/**==wifi reset, called by wlan_platform ==*/
void host_platform_reset_wifi( bool reset_asserted )
{
  if (reset_asserted)
    MicoGpioOutputLow( (mico_gpio_t)WL_RESET );  
  else
    MicoGpioOutputHigh( (mico_gpio_t)WL_RESET );  
}

/**==wifi power, called by wlan_platform ==*/
void host_platform_power_wifi( bool power_enabled )
{
  
}
/**==platform(board) status LEDs==*/
void MicoSysLed(bool onoff)
{
 // Board_LED_Set(0, onoff);
}

void MicoRfLed(bool onoff)
{
 // Board_LED_Set(1, onoff);
}

void Mico2rdLED(bool onoff)
{
//  Board_LED_Set(2, onoff);
}

