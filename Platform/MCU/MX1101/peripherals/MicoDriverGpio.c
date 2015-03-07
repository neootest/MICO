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
#include "platform.h"
#include "platform_common_config.h"
#include "MicoDriverMapping.h"
#include "gpio.h"
#include "debug.h"

/******************************************************
*                    Constants
******************************************************/
#define NUMBER_OF_GPIO_IRQ_LINES (5)

/******************************************************
*                   Enumerations
******************************************************/

/******************************************************
*                 Type Definitions
******************************************************/

typedef mico_gpio_irq_trigger_t gpio_irq_trigger_t;
typedef mico_gpio_irq_handler_t gpio_irq_handler_t;

typedef struct
{
  uint8_t port;
  uint8_t pin;
  gpio_irq_handler_t handler;
  void*              arg;
} gpio_irq_data_t;

/******************************************************
*                    Structures
******************************************************/

/******************************************************
*               Variables Definitions
******************************************************/

static uint8_t gpio_irq_management_initted = 0;
static volatile gpio_irq_data_t gpio_irq_data[ NUMBER_OF_GPIO_IRQ_LINES ];

/******************************************************
*               Function Declarations
******************************************************/

/******************************************************
*               Function Definitions
******************************************************/

OSStatus MicoGpioInitialize( mico_gpio_t gpio, mico_gpio_config_t configuration )
{
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;
  
  MicoMcuPowerSaveConfig(false);

  GpioClrRegOneBit(GPIO_A_OUTDS + gpio_mapping[gpio].port , ((uint32_t)1 << gpio_mapping[gpio].pin) );

  if ( (configuration == INPUT_PULL_UP ) || (configuration == INPUT_HIGH_IMPEDANCE ) || (configuration == INPUT_PULL_DOWN )){
    GpioClrRegOneBit(GPIO_A_OE + gpio_mapping[gpio].port , ((uint32_t)1 << gpio_mapping[gpio].pin) );
    GpioSetRegOneBit(GPIO_A_IE + gpio_mapping[gpio].port , ((uint32_t)1 << gpio_mapping[gpio].pin) );
  }else{
    GpioClrRegOneBit(GPIO_A_IE + gpio_mapping[gpio].port , ((uint32_t)1 << gpio_mapping[gpio].pin) );
    GpioSetRegOneBit(GPIO_A_OE + gpio_mapping[gpio].port , ((uint32_t)1 << gpio_mapping[gpio].pin) );    
  }
  
  if ( (configuration == INPUT_PULL_UP ) || (configuration == OUTPUT_OPEN_DRAIN_PULL_UP ) )
  {
    GpioClrRegOneBit(GPIO_A_PU + gpio_mapping[gpio].port , ((uint32_t)1 << gpio_mapping[gpio].pin) );
    GpioClrRegOneBit(GPIO_A_PD + gpio_mapping[gpio].port , ((uint32_t)1 << gpio_mapping[gpio].pin) );
  }
  else if (configuration == INPUT_PULL_DOWN )
  {
    GpioSetRegOneBit(GPIO_A_PU + gpio_mapping[gpio].port , ((uint32_t)1 << gpio_mapping[gpio].pin) );
    GpioSetRegOneBit(GPIO_A_PD + gpio_mapping[gpio].port , ((uint32_t)1 << gpio_mapping[gpio].pin) );
  }
  else
  {
    GpioSetRegOneBit(GPIO_A_PU + gpio_mapping[gpio].port , ((uint32_t)1 << gpio_mapping[gpio].pin) );
    GpioClrRegOneBit(GPIO_A_PD + gpio_mapping[gpio].port , ((uint32_t)1 << gpio_mapping[gpio].pin) );
  }
  
  if(configuration == OUTPUT_PUSH_PULL)
    GpioSetRegOneBit(GPIO_A_OUTDS + gpio_mapping[gpio].port , ((uint32_t)1 << gpio_mapping[gpio].pin) );
  

  MicoMcuPowerSaveConfig(true);
  
  return kNoErr;
}

OSStatus MicoGpioFinalize( mico_gpio_t gpio )
{ 
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  MicoMcuPowerSaveConfig(false);
  
  MicoMcuPowerSaveConfig(true);
  
  return kNoErr;
}

OSStatus MicoGpioOutputHigh( mico_gpio_t gpio )
{
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  MicoMcuPowerSaveConfig(false);

  GpioSetRegOneBit(GPIO_A_OUT + gpio_mapping[gpio].port , ((uint32_t)1 << gpio_mapping[gpio].pin) );
  
  MicoMcuPowerSaveConfig(true);
  
  return kNoErr;
}

OSStatus MicoGpioOutputLow( mico_gpio_t gpio )
{
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  MicoMcuPowerSaveConfig(false);

  GpioClrRegOneBit(GPIO_A_OUT + gpio_mapping[gpio].port , ((uint32_t)1 << gpio_mapping[gpio].pin) );
  
  MicoMcuPowerSaveConfig(true);
  
  return kNoErr;
}

OSStatus MicoGpioOutputTrigger( mico_gpio_t gpio )
{
  uint32_t regValue;
  uint32_t mask = (uint32_t)1 << gpio_mapping[gpio].pin;

  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  MicoMcuPowerSaveConfig(false);

  regValue = GpioGetReg( GPIO_A_OUT + gpio_mapping[gpio].port );

  if( (regValue&mask) )
    MicoGpioOutputLow( gpio );
  else
    MicoGpioOutputHigh( gpio );
  
  MicoMcuPowerSaveConfig(true);
  
  return kNoErr;  
}

bool MicoGpioInputGet( mico_gpio_t gpio )
{
  bool result;
  uint32_t regValue;
  uint32_t mask = (uint32_t)1 << gpio_mapping[gpio].pin;
  
  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  MicoMcuPowerSaveConfig(false);

  regValue = GpioGetReg( GPIO_A_IN + gpio_mapping[gpio].port );

  result =  ((regValue&mask)  == 0 )? false : true;
  
  MicoMcuPowerSaveConfig(true);
  
  return result;
}

OSStatus MicoGpioEnableIRQ( mico_gpio_t gpio, mico_gpio_irq_trigger_t trigger, mico_gpio_irq_handler_t handler, void* arg )
{
  uint8_t intPort;
  int i;

  if( trigger== IRQ_TRIGGER_BOTH_EDGES || gpio == (mico_gpio_t)MICO_GPIO_UNUSED )
    return kUnsupportedErr;
  
  if ( gpio_irq_management_initted == 0 )
  {
    memset( (void*)gpio_irq_data, 0, sizeof( gpio_irq_data ) );
    NVIC_EnableIRQ(GPIO_IRQn);
    gpio_irq_management_initted = 1;
  }

  switch( gpio_mapping[gpio].port ){
    case GPIOA:
      intPort = GPIO_A_INT;
      break;
    case GPIOB:
      intPort = GPIO_B_INT;
      break;
    case GPIOC:
      intPort = GPIO_C_INT;
      break;
    default:
      return kUnsupportedErr;
  }
    
  for( i = 0; i < NUMBER_OF_GPIO_IRQ_LINES; i++ ){
    if ( gpio_irq_data[i].port ==  gpio_mapping[gpio].port && gpio_irq_data[i].pin ==  gpio_mapping[gpio].pin){
      /* GPIO IRQ already exist */
      gpio_irq_data[ i ].handler    = handler;
      gpio_irq_data[ i ].arg        = arg;
      break;
    }
  }

  if(i == NUMBER_OF_GPIO_IRQ_LINES){
    /* GPIO IRQ not exist */
    for( i = 0; i < NUMBER_OF_GPIO_IRQ_LINES; i++ ){
      if ( gpio_irq_data[i].handler == NULL ){
        gpio_irq_data[ i ].port       = gpio_mapping[gpio].port;
        gpio_irq_data[ i ].pin        = gpio_mapping[gpio].pin;
        gpio_irq_data[ i ].handler    = handler;
        gpio_irq_data[ i ].arg        = arg;
        break;
      }
    } 
    /* No space to add one */
    if( i == NUMBER_OF_GPIO_IRQ_LINES)
      return kNoSpaceErr;
  }

  MicoMcuPowerSaveConfig(false);

  GpioIntClr(intPort, ((uint32_t)1 << gpio_mapping[gpio].pin));

  if( trigger == IRQ_TRIGGER_RISING_EDGE )
    GpioIntEn(intPort, ((uint32_t)1 << gpio_mapping[gpio].pin), GPIO_POS_EDGE_TRIGGER);
  else 
    GpioIntEn(intPort, ((uint32_t)1 << gpio_mapping[gpio].pin), GPIO_NEG_EDGE_TRIGGER);

  MicoMcuPowerSaveConfig(true);

  return kNoErr;
}

OSStatus MicoGpioDisableIRQ( mico_gpio_t gpio )
{
  uint8_t intPort;
  int i;

  if(gpio == (mico_gpio_t)MICO_GPIO_UNUSED ) return kUnsupportedErr;

  switch( gpio_mapping[gpio].port ){
  case GPIOA:
    intPort = GPIO_A_INT;
    break;
  case GPIOB:
    intPort = GPIO_B_INT;
    break;
  case GPIOC:
    intPort = GPIO_C_INT;
    break;
  default:
    return kUnsupportedErr;
  }

  MicoMcuPowerSaveConfig( false );

  GpioIntDis( intPort, ((uint32_t)1 << gpio_mapping[gpio].pin) );

  MicoMcuPowerSaveConfig( true );

  for( i = 0; i < NUMBER_OF_GPIO_IRQ_LINES; i++ ){
    if ( gpio_irq_data[i].port ==  gpio_mapping[gpio].port && gpio_irq_data[i].pin ==  gpio_mapping[gpio].pin){
      gpio_irq_data[ i ].port       = 0;
      gpio_irq_data[ i ].pin        = 0;
      gpio_irq_data[ i ].handler    = NULL;
      gpio_irq_data[ i ].arg        = NULL;
      break;
    }
  }

  if( i == NUMBER_OF_GPIO_IRQ_LINES)
    return kNotFoundErr;

  return kNoErr;
}


void GpioInterrupt(void)
{
  uint32_t intFlagA, intFlagB, intFlagC;
  int i;
  uint8_t port, pin;
  void * arg;

  intFlagA = GpioIntFlagGet( GPIO_A_INT );
  intFlagB = GpioIntFlagGet( GPIO_B_INT );
  intFlagC = GpioIntFlagGet( GPIO_C_INT );

  for( i = 0; i < NUMBER_OF_GPIO_IRQ_LINES; i++ ){
    if ( gpio_irq_data[i].handler != NULL ){
      port = gpio_irq_data[i].port;
      pin = gpio_irq_data[i].pin;

      switch( port ){
      case GPIOA:
        if( intFlagA & ((uint32_t)1<<pin) ){
          GpioIntClr(GPIO_A_INT, ((uint32_t)1<<pin));
          arg = gpio_irq_data[i].arg; 
          gpio_irq_data[i].handler( arg );
         
        }
        break;
      case GPIOB:
        if( intFlagB & ((uint32_t)1<<pin) ){
          GpioIntClr(GPIO_B_INT, ((uint32_t)1<<pin));
          arg = gpio_irq_data[i].arg; 
          gpio_irq_data[i].handler( arg );
          
        }
        break;
      case GPIOC:
        if( intFlagC & ((uint32_t)1<<pin) ){
          GpioIntClr(GPIO_C_INT, ((uint32_t)1<<pin));
          arg = gpio_irq_data[i].arg; 
          gpio_irq_data[i].handler( arg );
          
        }
        break;
      default:
        continue;
      }
    }
  }
}








