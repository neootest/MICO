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

#include "stdio.h"
#include "string.h"

#include "MICOPlatform.h"
#include "platform.h"
#include "stm32f2xx_platform.h"
#include "PlatformLogging.h"

/******************************************************
*                      Macros
******************************************************/

#ifdef __GNUC__
#define WEAK __attribute__ ((weak))
#elif defined ( __IAR_SYSTEMS_ICC__ )
#define WEAK __weak
#endif /* ifdef __GNUC__ */

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

/******************************************************
*               Variables Definitions
******************************************************/

/* This table maps STM32 pins to GPIO definitions on the schematic
* A full pin definition is provided in <WICED-SDK>/include/platforms/BCM943362WCD4/platform.h
*/

static uint32_t _default_start_time = 0;
static mico_timer_t _button_EL_timer;

const platform_pin_mapping_t gpio_mapping[] =
{
  /* Common GPIOs for internal use */
  [MICO_GPIO_WLAN_POWERSAVE_CLOCK]    = {WL_32K_OUT_BANK, WL_32K_OUT_PIN, WL_32K_OUT_BANK_CLK},
  [WL_GPIO0]                          = {GPIOB,  5,  RCC_AHB1Periph_GPIOB},
  [WL_GPIO1]                          = {GPIOA,  2,  RCC_AHB1Periph_GPIOB},
  [WL_RESET]                          = {GPIOA,  3,  RCC_AHB1Periph_GPIOA},
  [MICO_SYS_LED]                      = {GPIOB,  7,  RCC_AHB1Periph_GPIOB}, 
  [MICO_RF_LED]                       = {GPIOB,  6,  RCC_AHB1Periph_GPIOB},
  [BOOT_SEL]                          = {GPIOB,  1,  RCC_AHB1Periph_GPIOB}, 
  [MFG_SEL]                           = {GPIOB,  9,  RCC_AHB1Periph_GPIOB}, 
  [Standby_SEL]                       = {GPIOA,  0,  RCC_AHB1Periph_GPIOA}, 
  [EasyLink_BUTTON]                   = {GPIOB,  8,  RCC_AHB1Periph_GPIOB}, 
  [STDIO_UART_RX]                     = {GPIOA,  9,  RCC_AHB1Periph_GPIOA},
  [STDIO_UART_TX]                     = {GPIOA, 10,  RCC_AHB1Periph_GPIOA},
  [STDIO_UART_CTS]                    = {GPIOA, 12,  RCC_AHB1Periph_GPIOA},  
  [STDIO_UART_RTS]                    = {GPIOA, 11,  RCC_AHB1Periph_GPIOA},

  /* GPIOs for external use */
  [MICO_GPIO_0]  = {GPIOC,  6,  RCC_AHB1Periph_GPIOC},
  [MICO_GPIO_1]  = {GPIOC,  7,  RCC_AHB1Periph_GPIOC},
  [MICO_GPIO_2]  = {GPIOC,  8,  RCC_AHB1Periph_GPIOC},
  [MICO_GPIO_3]  = {GPIOC,  9,  RCC_AHB1Periph_GPIOC},
  [MICO_GPIO_4]  = {GPIOC,  12,  RCC_AHB1Periph_GPIOC},
  [MICO_GPIO_5]  = {GPIOC,  13,  RCC_AHB1Periph_GPIOC},
  [MICO_GPIO_6]  = {GPIOC,  10,  RCC_AHB1Periph_GPIOC},
  [MICO_GPIO_7]  = {GPIOC , 11,  RCC_AHB1Periph_GPIOC},
  [MICO_GPIO_8]  = {GPIOD,  2,  RCC_AHB1Periph_GPIOD},
//  [MICO_GPIO_9]
  [MICO_GPIO_10] = {GPIOA,  4,  RCC_AHB1Periph_GPIOA},
  [MICO_GPIO_11] = {GPIOA,  7,  RCC_AHB1Periph_GPIOA},
  [MICO_GPIO_12] = {GPIOA,  6,  RCC_AHB1Periph_GPIOA},
  [MICO_GPIO_13] = {GPIOA,  5,  RCC_AHB1Periph_GPIOA},
//  [MICO_GPIO_14]
  [MICO_GPIO_15] = {GPIOB,  2,  RCC_AHB1Periph_GPIOB},
  [MICO_GPIO_16] = {GPIOB,  10,  RCC_AHB1Periph_GPIOB},
  [MICO_GPIO_17] = {GPIOB,  11,  RCC_AHB1Periph_GPIOB},
  [MICO_GPIO_18] = {GPIOC,  5,  RCC_AHB1Periph_GPIOC},
  [MICO_GPIO_19] = {GPIOC,  4,  RCC_AHB1Periph_GPIOC},
  [MICO_GPIO_20] = {GPIOC,  3,  RCC_AHB1Periph_GPIOC},
  [MICO_GPIO_21] = {GPIOC,  2,  RCC_AHB1Periph_GPIOC},
  [MICO_GPIO_22] = {GPIOC,  1,  RCC_AHB1Periph_GPIOC},
  [MICO_GPIO_23] = {GPIOC,  0,  RCC_AHB1Periph_GPIOC},
};

/*
* Possible compile time inputs:
* - Set which ADC peripheral to use for each ADC. All on one ADC allows sequential conversion on all inputs. All on separate ADCs allows concurrent conversion.
*/
/* TODO : These need fixing */
const platform_adc_mapping_t adc_mapping[] =
{
  [MICO_ADC_1] = {ADC1, ADC_Channel_10, RCC_APB2Periph_ADC1, 1, (platform_pin_mapping_t*)&gpio_mapping[MICO_GPIO_23]},
  [MICO_ADC_2] = {ADC1, ADC_Channel_11, RCC_APB2Periph_ADC1, 1, (platform_pin_mapping_t*)&gpio_mapping[MICO_GPIO_22]},
  [MICO_ADC_3] = {ADC1, ADC_Channel_12, RCC_APB2Periph_ADC1, 1, (platform_pin_mapping_t*)&gpio_mapping[MICO_GPIO_21]},
  [MICO_ADC_4] = {ADC1, ADC_Channel_13, RCC_APB2Periph_ADC1, 1, (platform_pin_mapping_t*)&gpio_mapping[MICO_GPIO_20]},
  [MICO_ADC_5] = {ADC1, ADC_Channel_14, RCC_APB2Periph_ADC1, 1, (platform_pin_mapping_t*)&gpio_mapping[MICO_GPIO_19]},
  [MICO_ADC_6] = {ADC1, ADC_Channel_15, RCC_APB2Periph_ADC1, 1, (platform_pin_mapping_t*)&gpio_mapping[MICO_GPIO_18]},
};


/* PWM mappings */
const platform_pwm_mapping_t pwm_mappings[] =
{
#if ( MICO_WLAN_POWERSAVE_CLOCK_SOURCE == MICO_WLAN_POWERSAVE_CLOCK_IS_PWM )
  /* Extended PWM for internal use */
  [WICED_PWM_WLAN_POWERSAVE_CLOCK] = {TIM1, 1, RCC_APB2Periph_TIM1, GPIO_AF_TIM1, (platform_pin_mapping_t*)&gpio_mapping[WICED_GPIO_WLAN_POWERSAVE_CLOCK] }, /* or TIM2/Ch2                       */
#endif  
  [MICO_PWM_1]  = {TIM3, 4, RCC_APB1Periph_TIM3, GPIO_AF_TIM3, (platform_pin_mapping_t*)&gpio_mapping[MICO_GPIO_3]},    
  /* TODO: fill in the other options here ... */
};

const platform_spi_mapping_t spi_mapping[] =
{
  [MICO_SPI_1]  =
  {
    .spi_regs              = SPI1,
    .gpio_af               = GPIO_AF_SPI1,
    .peripheral_clock_reg  = RCC_APB2Periph_SPI1,
    .peripheral_clock_func = RCC_APB2PeriphClockCmd,
    .pin_mosi              = &gpio_mapping[MICO_GPIO_11],
    .pin_miso              = &gpio_mapping[MICO_GPIO_12],
    .pin_clock             = &gpio_mapping[MICO_GPIO_13],
    .tx_dma_stream         = DMA2_Stream5,
    .rx_dma_stream         = DMA2_Stream0,
    .tx_dma_channel        = DMA_Channel_3,
    .rx_dma_channel        = DMA_Channel_3,
    .tx_dma_stream_number  = 5,
    .rx_dma_stream_number  = 0
  }
};

const platform_uart_mapping_t uart_mapping[] =
{
  [MICO_UART_1] =
  {
    .usart                        = USART1,
    .gpio_af                      = GPIO_AF_USART1,
    .pin_tx                       = &gpio_mapping[STDIO_UART_TX],
    .pin_rx                       = &gpio_mapping[STDIO_UART_RX],
    .pin_cts                      = &gpio_mapping[STDIO_UART_CTS],
    .pin_rts                      = &gpio_mapping[STDIO_UART_RTS],
    .usart_peripheral_clock       = RCC_APB2Periph_USART1,
    .usart_peripheral_clock_func  = RCC_APB2PeriphClockCmd,
    .usart_irq                    = USART1_IRQn,
    .tx_dma                       = DMA2,
    .tx_dma_stream                = DMA2_Stream7,
    .tx_dma_stream_number         = 7,
    .tx_dma_channel               = DMA_Channel_4,
    .tx_dma_peripheral_clock      = RCC_AHB1Periph_DMA2,
    .tx_dma_peripheral_clock_func = RCC_AHB1PeriphClockCmd,
    .tx_dma_irq                   = DMA2_Stream7_IRQn,
    .rx_dma                       = DMA2,
    .rx_dma_stream                = DMA2_Stream2,
    .rx_dma_stream_number         = 2,
    .rx_dma_channel               = DMA_Channel_4,
    .rx_dma_peripheral_clock      = RCC_AHB1Periph_DMA2,
    .rx_dma_peripheral_clock_func = RCC_AHB1PeriphClockCmd,
    .rx_dma_irq                   = DMA2_Stream2_IRQn,
  },
  [MICO_UART_2] =
  {
    .usart                        = USART6,
    .gpio_af                      = GPIO_AF_USART6,
    .pin_tx                       = &gpio_mapping[MICO_GPIO_0],
    .pin_rx                       = &gpio_mapping[MICO_GPIO_1],
    .pin_cts                      = NULL,
    .pin_rts                      = NULL,
    .usart_peripheral_clock       = RCC_APB2Periph_USART6,
    .usart_peripheral_clock_func  = RCC_APB2PeriphClockCmd,
    .usart_irq                    = USART6_IRQn,
    .tx_dma                       = DMA2,
    .tx_dma_stream                = DMA2_Stream6,
    .tx_dma_channel               = DMA_Channel_5,
    .tx_dma_peripheral_clock      = RCC_AHB1Periph_DMA2,
    .tx_dma_peripheral_clock_func = RCC_AHB1PeriphClockCmd,
    .tx_dma_irq                   = DMA2_Stream6_IRQn,
    .rx_dma                       = DMA2,
    .rx_dma_stream                = DMA2_Stream1,
    .rx_dma_channel               = DMA_Channel_5,
    .rx_dma_peripheral_clock      = RCC_AHB1Periph_DMA2,
    .rx_dma_peripheral_clock_func = RCC_AHB1PeriphClockCmd,
    .rx_dma_irq                   = DMA2_Stream1_IRQn,
  },
};

 const platform_i2c_mapping_t i2c_mapping[] =
 {
  [MICO_I2C_1] =
  {
    .i2c = I2C2,
    .pin_scl                 = &gpio_mapping[MICO_GPIO_16],
    .pin_sda                 = &gpio_mapping[MICO_GPIO_17],
    .peripheral_clock_reg    = RCC_APB1Periph_I2C2,
    .tx_dma                  = DMA1,
    .tx_dma_peripheral_clock = RCC_AHB1Periph_DMA1,
    .tx_dma_stream           = DMA1_Stream7,
    .rx_dma_stream           = DMA1_Stream5,
    .tx_dma_stream_id        = 7,
    .rx_dma_stream_id        = 5,
    .tx_dma_channel          = DMA_Channel_1,
    .rx_dma_channel          = DMA_Channel_1,
    .gpio_af                 = GPIO_AF_I2C2
  },
};

/******************************************************
*               Function Definitions
******************************************************/

static void _button_EL_irq_handler( void* arg )
{
  (void)(arg);
  int interval = -1;
  
  if ( MicoGpioInputGet( (mico_gpio_t)EasyLink_BUTTON ) == 0 ) {
    _default_start_time = mico_get_time()+1;
    mico_start_timer(&_button_EL_timer);
  } else {
    interval = mico_get_time() + 1 - _default_start_time;
    if ( (_default_start_time != 0) && interval > 50 && interval < RestoreDefault_TimeOut){
      /* EasyLink button clicked once */
      PlatformEasyLinkButtonClickedCallback();
    }
    mico_stop_timer(&_button_EL_timer);
    _default_start_time = 0;
  }
}

static void _button_STANDBY_irq_handler( void* arg )
{
  (void)(arg);
  PlatformStandbyButtonClickedCallback();
}

static void _button_EL_Timeout_handler( void* arg )
{
  (void)(arg);
  _default_start_time = 0;
  PlatformEasyLinkButtonLongPressedCallback();
}

bool watchdog_check_last_reset( void )
{
  if ( RCC->CSR & RCC_CSR_WDGRSTF )
  {
    /* Clear the flag and return */
    RCC->CSR |= RCC_CSR_RMVF;
    return true;
  }
  
  return false;
}

OSStatus mico_platform_init( void )
{
  platform_log( "Platform initialised" );
  
  if ( true == watchdog_check_last_reset() )
  {
    platform_log( "WARNING: Watchdog reset occured previously. Please see watchdog.c for debugging instructions." );
  }
  
  return kNoErr;
}

void init_platform( void )
{
  MicoGpioInitialize( (mico_gpio_t)MICO_SYS_LED, OUTPUT_PUSH_PULL );
  MicoGpioOutputHigh( (mico_gpio_t)MICO_SYS_LED );
  MicoGpioInitialize( (mico_gpio_t)MICO_RF_LED, OUTPUT_OPEN_DRAIN_NO_PULL );
  MicoGpioOutputHigh( (mico_gpio_t)MICO_RF_LED );
  
  //  Initialise EasyLink buttons
  MicoGpioInitialize( (mico_gpio_t)EasyLink_BUTTON, INPUT_PULL_UP );
  mico_init_timer(&_button_EL_timer, RestoreDefault_TimeOut, _button_EL_Timeout_handler, NULL);
  MicoGpioEnableIRQ( (mico_gpio_t)EasyLink_BUTTON, IRQ_TRIGGER_BOTH_EDGES, _button_EL_irq_handler, NULL );
  
  //  Initialise Standby/wakeup switcher
  MicoGpioInitialize( (mico_gpio_t)Standby_SEL, INPUT_PULL_UP );
  MicoGpioEnableIRQ( (mico_gpio_t)Standby_SEL , IRQ_TRIGGER_FALLING_EDGE, _button_STANDBY_irq_handler, NULL);

  MicoFlashInitialize( MICO_SPI_FLASH );
}

void init_platform_bootloader( void )
{
  MicoGpioInitialize( (mico_gpio_t)MICO_SYS_LED, OUTPUT_PUSH_PULL );
  MicoGpioOutputHigh( (mico_gpio_t)MICO_SYS_LED );
  MicoGpioInitialize( (mico_gpio_t)MICO_RF_LED, OUTPUT_OPEN_DRAIN_NO_PULL );
  MicoGpioOutputHigh( (mico_gpio_t)MICO_RF_LED );
  
  MicoGpioInitialize((mico_gpio_t)BOOT_SEL, INPUT_PULL_UP);
  MicoGpioInitialize((mico_gpio_t)MFG_SEL, INPUT_HIGH_IMPEDANCE);
}

void host_platform_reset_wifi( bool reset_asserted )
{
  if ( reset_asserted == true )
  {
    MicoGpioOutputLow( (mico_gpio_t)WL_RESET );  
  }
  else
  {
    MicoGpioOutputHigh( (mico_gpio_t)WL_RESET ); 
  }
}

void host_platform_power_wifi( bool power_enabled )
{
  if ( power_enabled == true )
  {
    MicoGpioOutputLow( (mico_gpio_t)WL_REG );
  }
  else
  {
    MicoGpioOutputHigh( (mico_gpio_t)WL_REG ); 
  }
}

void MicoSysLed(bool onoff)
{
    if (onoff) {
        MicoGpioOutputLow( (mico_gpio_t)MICO_SYS_LED );
    } else {
        MicoGpioOutputHigh( (mico_gpio_t)MICO_SYS_LED );
    }
}

void MicoRfLed(bool onoff)
{
    if (onoff) {
        MicoGpioOutputLow( (mico_gpio_t)MICO_RF_LED );
    } else {
        MicoGpioOutputHigh( (mico_gpio_t)MICO_RF_LED );
    }
}

bool MicoShouldEnterMFGMode(void)
{
  if(MicoGpioInputGet((mico_gpio_t)BOOT_SEL)==false && MicoGpioInputGet((mico_gpio_t)MFG_SEL)==false)
    return true;
  else
    return false;
}

bool MicoShouldEnterBootloader(void)
{
  if(MicoGpioInputGet((mico_gpio_t)BOOT_SEL)==false && MicoGpioInputGet((mico_gpio_t)MFG_SEL)==true)
    return true;
  else
    return false;
}


