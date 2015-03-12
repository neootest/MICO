/**
******************************************************************************
* @file    MicoDriverGpio.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide GPIO driver functions.
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
#include "MICORTOS.h"
#include "gpio_irq.h"

#include "platform.h"
#include "platform_common_config.h"
#include "stm32f4xx_platform.h"
#include "stm32f4xx.h"

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
*               Variables Definitions
******************************************************/

/******************************************************
*               Function Declarations
******************************************************/

/******************************************************
*               Function Definitions
******************************************************/

OSStatus MicoGpioInitialize( mico_gpio_t gpio, mico_gpio_config_t configuration )
{
  GPIO_InitTypeDef gpio_init_structure;

  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;
  
  MicoMcuPowerSaveConfig(false);
  
  RCC_AHB1PeriphClockCmd( gpio_mapping[gpio].peripheral_clock, ENABLE );
  gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
  gpio_init_structure.GPIO_Mode  = ( (configuration == INPUT_PULL_UP ) || (configuration == INPUT_PULL_DOWN ) || (configuration == INPUT_HIGH_IMPEDANCE ) ) ? GPIO_Mode_IN : GPIO_Mode_OUT;
  gpio_init_structure.GPIO_OType = (configuration == OUTPUT_PUSH_PULL )? GPIO_OType_PP :  GPIO_OType_OD;
  
  if ( (configuration == INPUT_PULL_UP ) || (configuration == OUTPUT_OPEN_DRAIN_PULL_UP ) )
  {
    gpio_init_structure.GPIO_PuPd  =  GPIO_PuPd_UP;
  }
  else if (configuration == INPUT_PULL_DOWN )
  {
    gpio_init_structure.GPIO_PuPd  =  GPIO_PuPd_DOWN;
  }
  else
  {
    gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  }
  
  gpio_init_structure.GPIO_Pin = (uint16_t) ( 1 << gpio_mapping[gpio].number );
  GPIO_Init( gpio_mapping[gpio].bank, &gpio_init_structure );
  
  MicoMcuPowerSaveConfig(true);
  
  return kNoErr;
}

OSStatus MicoGpioFinalize( mico_gpio_t gpio )
{
  GPIO_InitTypeDef gpio_init_structure;
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;
  
  MicoMcuPowerSaveConfig(false);
 
  gpio_init_structure.GPIO_Mode  = GPIO_Mode_IN;
  gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  
  gpio_init_structure.GPIO_Pin = (uint16_t) ( 1 << gpio_mapping[gpio].number );
  GPIO_Init( gpio_mapping[gpio].bank, &gpio_init_structure );
  
  MicoGpioDisableIRQ( gpio );
  
  MicoMcuPowerSaveConfig(true);
  
  return kNoErr;
}

OSStatus MicoGpioOutputHigh( mico_gpio_t gpio )
{
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  MicoMcuPowerSaveConfig(false);
  
  GPIO_SetBits( gpio_mapping[gpio].bank, (uint16_t) (1 << gpio_mapping[gpio].number) );
  
  MicoMcuPowerSaveConfig(true);
  
  return kNoErr;
}

OSStatus MicoGpioOutputLow( mico_gpio_t gpio )
{
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  MicoMcuPowerSaveConfig(false);
  
  GPIO_ResetBits( gpio_mapping[gpio].bank, (uint16_t) (1 << gpio_mapping[gpio].number) );
  
  MicoMcuPowerSaveConfig(true);
  
  return kNoErr;
}

OSStatus MicoGpioOutputTrigger( mico_gpio_t gpio )
{
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  MicoMcuPowerSaveConfig(false);
  
  GPIO_ToggleBits( gpio_mapping[gpio].bank, (uint16_t) (1 << gpio_mapping[gpio].number) );
  
  MicoMcuPowerSaveConfig(true);
  
  return kNoErr;    
}

bool MicoGpioInputGet( mico_gpio_t gpio )
{
  bool result;
  
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  MicoMcuPowerSaveConfig(false);
  
  result =  ( GPIO_ReadInputDataBit( gpio_mapping[gpio].bank, (uint16_t) ( 1 << gpio_mapping[gpio].number ) ) == 0 )? false : true;
  
  MicoMcuPowerSaveConfig(true);
  
  return result;
}

OSStatus MicoGpioEnableIRQ( mico_gpio_t gpio, mico_gpio_irq_trigger_t trigger, mico_gpio_irq_handler_t handler, void* arg )
{
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  return gpio_irq_enable( gpio_mapping[gpio].bank, gpio_mapping[gpio].number, trigger, handler, arg );
}

OSStatus MicoGpioDisableIRQ( mico_gpio_t gpio )
{
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;
  
  return gpio_irq_disable( gpio_mapping[gpio].bank, gpio_mapping[gpio].number );
}
