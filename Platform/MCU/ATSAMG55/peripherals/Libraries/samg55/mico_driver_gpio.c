/**
******************************************************************************
* @file    mico_driver_gpio.c 
* @author  william xu
* @version V1.0.0
* @date    05-may-2014
* @brief   this file provide GPIO driver functions.
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


#include "mico_platform.h"
#include "mico_rtos.h"

#include "platform.h"
#include "mico_common_driver.h"
#include "gpio_irq.h"

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
*               variables definitions
******************************************************/

/******************************************************
*               function declarations
******************************************************/

/******************************************************
*               function definitions
******************************************************/

OSStatus mico_gpio_initialize( mico_gpio_t gpio, mico_gpio_config_t configuration )
{
  ioport_mode_t mode = 0; 
  ioport_pin_t  pin = 0;
  enum ioport_direction dir;

  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;
  
  mico_mcu_power_save_config(false);
  
  dir = ( (configuration == INPUT_PULL_UP ) || (configuration == INPUT_PULL_DOWN ) || (configuration == INPUT_HIGH_IMPEDANCE ) ) ? IOPORT_DIR_INPUT: IOPORT_DIR_OUTPUT;

  if ( (configuration == INPUT_PULL_UP ) || (configuration == OUTPUT_OPEN_DRAIN_PULL_UP ) )
  {
      mode |= IOPORT_MODE_PULLUP;              
  }
  else if (configuration == INPUT_PULL_DOWN )
  {
      mode |= IOPORT_MODE_PULLDOWN;
  }
  else if ( (configuration == OUTPUT_OPEN_DRAIN_PULL_UP ) || (configuration == OUTPUT_OPEN_DRAIN_NO_PULL) )
  {
      mode |= IOPORT_MODE_OPEN_DRAIN;
  }
// other input debounce ,glitch filter; 
  pin = CREATE_IOPORT_PIN(gpio_mapping[gpio].bank, gpio_mapping[gpio].number);

  ioport_set_pin_dir(pin, dir);
  ioport_set_pin_mode(pin, mode);
  
  mico_mcu_power_save_config(true);
  
  return kNoErr;
}

OSStatus mico_gpio_finalize( mico_gpio_t gpio )
{
  ioport_pin_t  pin = 0;
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;
  
  mico_mcu_power_save_config(false);
 
  pin = CREATE_IOPORT_PIN(gpio_mapping[gpio].bank, gpio_mapping[gpio].number);

  ioport_disable_pin(pin);
 
  mico_gpio_disable_IRQ( gpio );
  
  mico_mcu_power_save_config(true);
  
  return kNoErr;
}

OSStatus mico_gpio_output_high( mico_gpio_t gpio )
{
  ioport_pin_t  pin = 0;
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  mico_mcu_power_save_config(false);
  
  pin = CREATE_IOPORT_PIN(gpio_mapping[gpio].bank, gpio_mapping[gpio].number);
  
  ioport_set_pin_level(pin, IOPORT_PIN_LEVEL_HIGH);
  
  mico_mcu_power_save_config(true);
  
  return kNoErr;
}

OSStatus mico_gpio_output_low( mico_gpio_t gpio )
{
  ioport_pin_t  pin = 0;
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  mico_mcu_power_save_config(false);
  
  pin = CREATE_IOPORT_PIN(gpio_mapping[gpio].bank, gpio_mapping[gpio].number);
  
  ioport_set_pin_level(pin, IOPORT_PIN_LEVEL_LOW);
  
  mico_mcu_power_save_config(true);
  
  return kNoErr;
}

OSStatus mico_gpio_output_trigger( mico_gpio_t gpio )
{
  ioport_pin_t  pin = 0;
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  mico_mcu_power_save_config(false);
  
  pin = CREATE_IOPORT_PIN(gpio_mapping[gpio].bank, gpio_mapping[gpio].number);
  
  ioport_toggle_pin_level(pin);
  
  mico_mcu_power_save_config(true);
  
  return kNoErr;    
}

bool mico_gpio_input_get( mico_gpio_t gpio )
{
  bool result;
  ioport_pin_t  pin = 0;
  
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  mico_mcu_power_save_config(false);
  
  pin = CREATE_IOPORT_PIN(gpio_mapping[gpio].bank, gpio_mapping[gpio].number);
  
  result = ioport_get_pin_level(pin) == 0 ? false : true;
  
  mico_mcu_power_save_config(true);
  
  return result;
}

OSStatus mico_gpio_enable_IRQ( mico_gpio_t gpio, mico_gpio_irq_trigger_t trigger, mico_gpio_irq_handler_t handler, void* arg )
{
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  return gpio_irq_enable( gpio_mapping[gpio].bank, gpio_mapping[gpio].number, trigger, handler, arg );
}

OSStatus mico_gpio_disable_IRQ( mico_gpio_t gpio )
{
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;
  
  return gpio_irq_disable( gpio_mapping[gpio].bank, gpio_mapping[gpio].number );
}
