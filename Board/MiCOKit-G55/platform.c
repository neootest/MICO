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

#include "platform.h"
#include "platform_config.h"
#include "platform_peripheral.h"
#include "PlatformLogging.h"
#include "MicoPlatform.h"
#include "wlan_platform_common.h"
#include "spi_flash_platform_interface.h"

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

/******************************************************
*               Variables Definitions
******************************************************/

/* This table maps STM32 pins to GPIO definitions on the schematic
* A full pin definition is provided in <WICED-SDK>/include/platforms/BCM943362WCD4/platform.h
*/

static uint32_t _default_start_time = 0;
static mico_timer_t _button_EL_timer;

const platform_gpio_t platform_gpio_pins[] =
{
  /* Common GPIOs for internal use */
  [STDIO_UART_TX]                       = { IOPORT_CREATE_PIN( PIOA, 28 ),  false, 0, 0  },
  [STDIO_UART_RX]                       = { IOPORT_CREATE_PIN( PIOA, 29 ),  false, 0, 0 },  

  [FLASH_PIN_SPI_CS  ]                  = { IOPORT_CREATE_PIN( PIOA, 19 ),  false, 0, 0 }, 
  [FLASH_PIN_SPI_CLK ]                  = { IOPORT_CREATE_PIN( PIOB, 13 ),  false, 0, 0 },
  [FLASH_PIN_SPI_MOSI]                  = { IOPORT_CREATE_PIN( PIOA,  3 ),  false, 0, 0 },
  [FLASH_PIN_SPI_MISO]                  = { IOPORT_CREATE_PIN( PIOA,  4 ),  false, 0, 0 },
 

  /* GPIOs for external use */
  [MICO_GPIO_0]                       = { IOPORT_CREATE_PIN( PIOA,  6 ),  false, 0, 0  },
  [MICO_GPIO_1]                       = { IOPORT_CREATE_PIN( PIOA,  2 ),  true,  2, IOPORT_SENSE_FALLING },
  [MICO_GPIO_2]                       = { IOPORT_CREATE_PIN( PIOB,  0 ),  false, 0, 0  },
  [MICO_GPIO_3]                       = { IOPORT_CREATE_PIN( PIOB,  1 ),  false, 0, 0 }, 

};

const platform_adc_t *platform_adc_peripherals = NULL;


/* PWM mappings */
const platform_pwm_t *platform_pwm_peripherals = NULL;

const platform_spi_t platform_spi_peripherals [] =  
{
  [MICO_SPI_1] =
  {
    .spi_id                       = 3,
    .port                         = SPI3,
    .peripheral_id                = ID_FLEXCOM3,
    .mosi_pin                     = &platform_gpio_pins[FLASH_PIN_SPI_MOSI],
    .mosi_pin_mux_mode            = IOPORT_MODE_MUX_A,
    .miso_pin                     = &platform_gpio_pins[FLASH_PIN_SPI_MISO],
    .miso_pin_mux_mode            = IOPORT_MODE_MUX_A,
    .clock_pin                    = &platform_gpio_pins[FLASH_PIN_SPI_CLK],
    .clock_pin_mux_mode           = IOPORT_MODE_MUX_A,
  },
};


platform_spi_driver_t *platform_spi_drivers = NULL ;

const platform_uart_t platform_uart_peripherals[] =
{
  [MICO_UART_1] =
  {
  .uart_id          = 7,
  .port             = USART7,
  .peripheral_id    = ID_FLEXCOM7,
  .tx_pin           = &platform_gpio_pins[STDIO_UART_TX],
  .tx_pin_mux_mode  = IOPORT_MODE_MUX_B,
  .rx_pin           = &platform_gpio_pins[STDIO_UART_RX],
  .rx_pin_mux_mode  = IOPORT_MODE_MUX_B,
  .cts_pin          = NULL, /* flow control isn't supported */
  .cts_pin_mux_mode = IOPORT_MODE_MUX_B,
  .rts_pin          = NULL, /* flow control isn't supported */
  .rts_pin_mux_mode = IOPORT_MODE_MUX_B,
  },
  [MICO_UART_2] =
  {
  .uart_id          = 6,
  .port             = USART6,
  .peripheral_id    = ID_FLEXCOM6,
  .tx_pin           = &platform_gpio_pins[MICO_GPIO_2],
  .tx_pin_mux_mode  = IOPORT_MODE_MUX_B,
  .rx_pin           = &platform_gpio_pins[MICO_GPIO_3],
  .rx_pin_mux_mode  = IOPORT_MODE_MUX_B,
  .cts_pin          = NULL, /* flow control isn't supported */
  .cts_pin_mux_mode = IOPORT_MODE_MUX_B,
  .rts_pin          = NULL, /* flow control isn't supported */
  .rts_pin_mux_mode = IOPORT_MODE_MUX_B,
  },
};

platform_uart_driver_t platform_uart_drivers[MICO_UART_MAX];


const platform_i2c_t *platform_i2c_peripherals = NULL;

const platform_flash_t platform_flash_peripherals[] =
{
  [MICO_SPI_FLASH] =
  {
    .flash_type                   = FLASH_TYPE_SPI,
    .flash_start_addr             = 0x000000,
    .flash_length                 = 0x200000,
  },
  [MICO_INTERNAL_FLASH] =
  {
    .flash_type                   = FLASH_TYPE_INTERNAL,
    .flash_start_addr             = 0x00400000,
    .flash_length                 = 0x80000,
  },
};

platform_flash_driver_t platform_flash_drivers[MICO_FLASH_MAX];

/* Wi-Fi control pins. Used by platform/MCU/wlan_platform_common.c
* SDIO: EMW1062_PIN_BOOTSTRAP[1:0] = b'00
* gSPI: EMW1062_PIN_BOOTSTRAP[1:0] = b'01
*/
const platform_gpio_t wifi_control_pins[] =
{
  [WIFI_PIN_RESET      ] = { IOPORT_CREATE_PIN( PIOA, 25 ),  false, 0, 0 },
  [WIFI_PIN_BOOTSTRAP_0] = { IOPORT_CREATE_PIN( PIOA, 26 ),  false, 0, 0 },
  [WIFI_PIN_BOOTSTRAP_1] = { IOPORT_CREATE_PIN( PIOA, 18 ),  false, 0, 0 },
};

/* Wi-Fi gSPI bus pins. Used by platform/MCU/STM32F2xx/EMW1062_driver/wlan_spi.c */
const platform_gpio_t wifi_spi_pins[] =
{
  [WIFI_PIN_SPI_IRQ ] = { IOPORT_CREATE_PIN( PIOA, 24 ),  true, 11, IOPORT_SENSE_RISING },
  [WIFI_PIN_SPI_CS  ] = { IOPORT_CREATE_PIN( PIOA, 11 ),  false, 0, 0 },
  [WIFI_PIN_SPI_CLK ] = { IOPORT_CREATE_PIN( PIOA, 14 ),  false, 0, 0 },
  [WIFI_PIN_SPI_MOSI] = { IOPORT_CREATE_PIN( PIOA, 13 ),  false, 0, 0 },
  [WIFI_PIN_SPI_MISO] = { IOPORT_CREATE_PIN( PIOA, 12 ),  false, 0, 0 },
};

const platform_spi_t wifi_spi =
{
  .spi_id                       = 5,
  .port                         = SPI5,
  .peripheral_id                = ID_FLEXCOM5,
  .mosi_pin                     = &wifi_spi_pins[WIFI_PIN_SPI_MOSI],
  .mosi_pin_mux_mode            = IOPORT_MODE_MUX_A,
  .miso_pin                     = &wifi_spi_pins[WIFI_PIN_SPI_MISO],
  .miso_pin_mux_mode            = IOPORT_MODE_MUX_A,
  .clock_pin                    = &wifi_spi_pins[WIFI_PIN_SPI_CLK],
  .clock_pin_mux_mode           = IOPORT_MODE_MUX_A,
};

#if defined ( USE_MICO_SPI_FLASH )
const mico_spi_device_t mico_spi_flash =
{
  .port        = MICO_SPI_1,
  .chip_select = FLASH_PIN_SPI_CS,
  .speed       = 40000000,
  .mode        = (SPI_CLOCK_RISING_EDGE | SPI_CLOCK_IDLE_HIGH | SPI_NO_DMA | SPI_MSB_FIRST),
  .bits        = 8,
};
#endif


/******************************************************
*           Interrupt Handler Definitions
******************************************************/

MICO_RTOS_DEFINE_ISR( FLEXCOM7_Handler )
{
    platform_uart_irq( &platform_uart_drivers[MICO_UART_1] );
}

MICO_RTOS_DEFINE_ISR( FLEXCOM6_Handler )
{
    platform_uart_irq( &platform_uart_drivers[MICO_UART_2] );
}

MICO_RTOS_DEFINE_ISR( FLEXCOM5_Handler )
{
    platform_wifi_spi_rx_dma_irq( );
}


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


static void _button_EL_Timeout_handler( void* arg )
{
  (void)(arg);
  _default_start_time = 0;
  PlatformEasyLinkButtonLongPressedCallback();
}

void platform_init_peripheral_irq_priorities( void )
{
  NVIC_SetPriority  ( PIOA_IRQn,      14 );
  NVIC_SetPriority  ( PIOB_IRQn,      14 );
  NVIC_SetPriority  ( FLEXCOM7_IRQn,   6 );  /* STDIO UART  */
  NVIC_SetPriority  ( FLEXCOM5_IRQn,   3 );  /* WLAN SPI    */
  NVIC_SetPriority  ( RTT_IRQn,        1 );  /* RTT Wake-up event */
//  NVIC_SetPriority( RTC_WKUP_IRQn    ,  1 ); /* RTC Wake-up event   */
//  NVIC_SetPriority( SDIO_IRQn        ,  2 ); /* WLAN SDIO           */
//  NVIC_SetPriority( DMA2_Stream3_IRQn,  3 ); /* WLAN SDIO DMA       */
//  NVIC_SetPriority( DMA1_Stream3_IRQn,  3 ); /* WLAN SPI DMA        */
//  NVIC_SetPriority( USART1_IRQn      ,  6 ); /* MICO_UART_1         */
//  NVIC_SetPriority( USART6_IRQn      ,  6 ); /* MICO_UART_2         */
//  NVIC_SetPriority( DMA2_Stream7_IRQn,  7 ); /* MICO_UART_1 TX DMA  */
//  NVIC_SetPriority( DMA2_Stream2_IRQn,  7 ); /* MICO_UART_1 RX DMA  */
//  NVIC_SetPriority( DMA2_Stream6_IRQn,  7 ); /* MICO_UART_2 TX DMA  */
//  NVIC_SetPriority( DMA2_Stream1_IRQn,  7 ); /* MICO_UART_2 RX DMA  */
//  NVIC_SetPriority( EXTI0_IRQn       , 14 ); /* GPIO                */
//  NVIC_SetPriority( EXTI1_IRQn       , 14 ); /* GPIO                */
//  NVIC_SetPriority( EXTI2_IRQn       , 14 ); /* GPIO                */
//  NVIC_SetPriority( EXTI3_IRQn       , 14 ); /* GPIO                */
//  NVIC_SetPriority( EXTI4_IRQn       , 14 ); /* GPIO                */
//  NVIC_SetPriority( EXTI9_5_IRQn     , 14 ); /* GPIO                */
//  NVIC_SetPriority( EXTI15_10_IRQn   , 14 ); /* GPIO                */
}

void init_platform( void )
{
//  MicoGpioInitialize( (mico_gpio_t)MICO_SYS_LED, OUTPUT_PUSH_PULL );
//  MicoGpioOutputHigh( (mico_gpio_t)MICO_SYS_LED );
//  MicoGpioInitialize( (mico_gpio_t)MICO_RF_LED, OUTPUT_OPEN_DRAIN_NO_PULL );
//  MicoGpioOutputHigh( (mico_gpio_t)MICO_RF_LED );
//  
  //  Initialise EasyLink buttons
  MicoGpioInitialize( (mico_gpio_t)EasyLink_BUTTON, INPUT_PULL_UP );
  mico_init_timer(&_button_EL_timer, RestoreDefault_TimeOut, _button_EL_Timeout_handler, NULL);
  MicoGpioEnableIRQ( (mico_gpio_t)EasyLink_BUTTON, IRQ_TRIGGER_BOTH_EDGES, _button_EL_irq_handler, NULL );

#if defined ( USE_MICO_SPI_FLASH )
  MicoFlashInitialize( MICO_SPI_FLASH );
#endif
}

void init_platform_bootloader( void )
{
  MicoGpioInitialize( (mico_gpio_t)MICO_SYS_LED, OUTPUT_PUSH_PULL );
  MicoGpioOutputHigh( (mico_gpio_t)MICO_SYS_LED );
  MicoGpioInitialize( (mico_gpio_t)MICO_RF_LED, OUTPUT_OPEN_DRAIN_NO_PULL );
  MicoGpioOutputHigh( (mico_gpio_t)MICO_RF_LED );
  
  MicoGpioInitialize(BOOT_SEL, INPUT_PULL_UP);
  MicoGpioInitialize(MFG_SEL, INPUT_PULL_UP);
  
#if defined ( USE_MICO_SPI_FLASH )
  MicoFlashInitialize( MICO_SPI_FLASH );
#endif
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
//  if(MicoGpioInputGet((mico_gpio_t)BOOT_SEL)==false && MicoGpioInputGet((mico_gpio_t)MFG_SEL)==true)
    return true;
//  else
//   return false;
}


