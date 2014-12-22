/**
******************************************************************************
* @file    MicoDriverUart.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide UART driver functions.
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


#include "MICORTOS.h"
#include "MICOPlatform.h"

#include "platform.h"
#include "platform_common_config.h"
#include "fsl_device_registers.h"
#include "fsl_uart_driver.h"
#include "fsl_edma_driver.h"
#include "fsl_uart_edma_driver.h"
#include "fsl_clock_manager.h"
#include "board.h"

#include "gpio_irq.h"

#include "string.h"
#include "PlatformLogging.h"
/******************************************************
*                    Constants
******************************************************/


/******************************************************
*                   Enumerations
******************************************************/

/******************************************************
*                 Type Definitions
******************************************************/
#define ADD_OS_CODE 0 //1 //test #macro
#define USE_DMA_UART
/* Ring buffer size */
#define UART_RB_SIZE 128


/* Transmit and receive buffers */
static uint8_t rxbuff[UART_RB_SIZE], txbuff[UART_RB_SIZE];

/******************************************************
*                    Structures
******************************************************/

typedef struct
{
  uint32_t            rx_size;
  ring_buffer_t*      rx_buffer;
#ifndef NO_MICO_RTOS
  mico_semaphore_t    rx_complete;
  mico_semaphore_t    tx_complete;
#else
  volatile bool       rx_complete;
  volatile bool       tx_complete;
#endif
  mico_semaphore_t    sem_wakeup;
  OSStatus            tx_dma_result;
  OSStatus            rx_dma_result;
} uart_interface_t;

/******************************************************
*               Variables Definitions
******************************************************/

static uart_interface_t uart_interfaces[NUMBER_OF_UART_INTERFACES];


static edma_state_t state_app; 
static edma_user_config_t userConfig_app; 
static uart_edma_state_t uartStateEdma_app;    
static uart_edma_user_config_t uartConfig_app;


#ifndef NO_MICO_RTOS
static mico_uart_t current_uart;
#endif

/******************************************************
*               Function Declarations
******************************************************/

static OSStatus internal_uart_init ( mico_uart_t uart, const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer );
static OSStatus platform_uart_receive_bytes( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout );


/* Interrupt service functions - called from interrupt vector table */
#ifndef NO_MICO_RTOS
static void thread_wakeup(void *arg);
static void RX_PIN_WAKEUP_handler(void *arg);
#endif


/******************************************************
*               Function Definitions
******************************************************/

OSStatus MicoUartInitialize( mico_uart_t uart, const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer )
{
  return internal_uart_init(uart, config, optional_rx_buffer);
  
}


OSStatus MicoStdioUartInitialize( const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer )
{
  return internal_uart_init(STDIO_UART, config, optional_rx_buffer);
}


OSStatus internal_uart_init( mico_uart_t uart, const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer )
{
    // uart_state_t uartState;
    /*edma_state_t state; 
    edma_user_config_t userConfig; 
    uart_edma_state_t uartStateEdma;    
    uart_edma_user_config_t uartConfig;*/
    uint32_t i;
         platform_log("internal uart init.");
#if ADD_OS_CODE
#ifndef NO_MICO_RTOS
  mico_rtos_init_semaphore(&uart_interfaces[uart].tx_complete, 1);
  mico_rtos_init_semaphore(&uart_interfaces[uart].rx_complete, 1);
#else
  uart_interfaces[uart].tx_complete = false;
  uart_interfaces[uart].rx_complete = false;
#endif  
#endif 
  MicoMcuPowerSaveConfig(false);  
    /* Configure the UART TX/RX pins */
    configure_uart_pins(BOARD_APP_UART_INSTANCE);
#if ADD_OS_CODE
#ifndef NO_MICO_RTOS
  if(config->flags & UART_WAKEUP_ENABLE){
    current_uart = uart;
    mico_rtos_init_semaphore( &uart_interfaces[uart].sem_wakeup, 1 );
    mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART_WAKEUP", thread_wakeup, 0x100, &current_uart);
  }
#endif 
#endif 
#if defined(USE_DMA_UART)
	//OSA_Init();
    userConfig_app.chnArbitration = kEDMAChnArbitrationRoundrobin;
    userConfig_app.notHaltOnError = false;

    uartConfig_app.bitCountPerChar = kUart8BitsPerChar;
    uartConfig_app.parityMode = kUartParityDisabled;
    uartConfig_app.stopBitCount = kUartOneStopBit;
    uartConfig_app.baudRate = 115200;

    EDMA_DRV_Init(&state_app, &userConfig_app);    
    UART_DRV_EdmaInit(BOARD_APP_UART_INSTANCE, &uartStateEdma_app, &uartConfig_app); 
   //  DMA0_IRQHandler();//test
    //uint8_t buff[] = "\n\r++++++++++++++ UART-DMA Test Start ++++++++++++++++++\n\r";//test
   //  uint32_t byteCountBRchange = sizeof(buff); //test
   //  UART_DRV_EdmaSendDataBlocking(BOARD_DEBUG_UART_INSTANCE, buff, byteCountBRchange, 2000); //test
  if (optional_rx_buffer != NULL)
  {
     //  Note that the ring_buffer should've been initialised first
    uart_interfaces[uart].rx_buffer = optional_rx_buffer;
    uart_interfaces[uart].rx_size   = 0;
    platform_uart_receive_bytes( uart, optional_rx_buffer->buffer, optional_rx_buffer->size, 0 );
  }  
#endif

  MicoMcuPowerSaveConfig(true);  
  return kNoErr;
}

OSStatus MicoUartFinalize( mico_uart_t uart )
{
  
    UART_DRV_EdmaDeinit(BOARD_APP_UART_INSTANCE);    
    EDMA_DRV_Deinit();    
#if ADD_OS_CODE
#ifndef NO_MICO_RTOS
  mico_rtos_deinit_semaphore(&uart_interfaces[uart].rx_complete);
  mico_rtos_deinit_semaphore(&uart_interfaces[uart].tx_complete);
#endif  
#endif  
  
  return kNoErr;
}

OSStatus MicoUartSend( mico_uart_t uart, const void* data, uint32_t size )
{
//  /* Reset DMA transmission result. The result is assigned in interrupt handler */
//   uart_interfaces[uart].tx_dma_result = kGeneralErr;
         platform_log("==== uart send.");
//  
  MicoMcuPowerSaveConfig(false);  
  if (UART_DRV_EdmaSendData(BOARD_APP_UART_INSTANCE, data, size) == kStatus_UART_Success){
#if ADD_OS_CODE
        #ifndef NO_MICO_RTOS
            mico_rtos_set_semaphore( &uart_interfaces[ uart ].tx_complete );
        #else
            uart_interfaces[ uart ].rx_complete = true;
        #endif
#endif
  }
//  
//  uart_mapping[uart].tx_dma_stream->CR  &= ~(uint32_t) DMA_SxCR_CIRC;
//  uart_mapping[uart].tx_dma_stream->NDTR = size;
//  uart_mapping[uart].tx_dma_stream->M0AR = (uint32_t)data;
//  
//  USART_DMACmd( uart_mapping[uart].usart, USART_DMAReq_Tx, ENABLE );
//  USART_ClearFlag( uart_mapping[uart].usart, USART_FLAG_TC );
//  DMA_Cmd( uart_mapping[uart].tx_dma_stream, ENABLE );
//  
#if ADD_OS_CODE
#ifndef NO_MICO_RTOS
  mico_rtos_get_semaphore( &uart_interfaces[ uart ].tx_complete, MICO_NEVER_TIMEOUT );
#else 
  while(uart_interfaces[ uart ].tx_complete == false);
  uart_interfaces[ uart ].tx_complete = false;
#endif
#endif
//  return uart_interfaces[uart].tx_dma_result; 
  MicoMcuPowerSaveConfig(true);

  return kNoErr;
}

OSStatus MicoUartRecv( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{
#if 0 
  if (uart_interfaces[uart].rx_buffer != NULL)
  {
    while (size != 0)
    {
      uint32_t transfer_size = MIN(uart_interfaces[uart].rx_buffer->size / 2, size);
      
      /* Check if ring buffer already contains the required amount of data. */
      if ( transfer_size > ring_buffer_used_space( uart_interfaces[uart].rx_buffer ) )
      {
        /* Set rx_size and wait in rx_complete semaphore until data reaches rx_size or timeout occurs */
        uart_interfaces[uart].rx_size = transfer_size;
#if ADD_OS_CODE
#ifndef NO_MICO_RTOS
        if ( mico_rtos_get_semaphore( &uart_interfaces[uart].rx_complete, timeout) != kNoErr )
        {
         platform_log("uart receive 01"); 
          uart_interfaces[uart].rx_size = 0;
          return kTimeoutErr;
        }
#else
        uart_interfaces[uart].rx_complete = false;
        int delay_start = mico_get_time_no_os();
        while(uart_interfaces[uart].rx_complete == false){
          if(mico_get_time_no_os() >= delay_start + timeout && timeout != MICO_NEVER_TIMEOUT){
            uart_interfaces[uart].rx_size = 0;
            return kTimeoutErr;
          }
        }
#endif
#endif
         platform_log("uart receive 02"); 
        /* Reset rx_size to prevent semaphore being set while nothing waits for the data */
        uart_interfaces[uart].rx_size = 0;
      }
     
      size -= transfer_size;
      
      // Grab data from the buffer
      do
      {
        uint8_t* available_data;
        uint32_t bytes_available;
        
         platform_log("uart receive 03"); 
        ring_buffer_get_data( uart_interfaces[uart].rx_buffer, &available_data, &bytes_available );
        bytes_available = MIN( bytes_available, transfer_size );
        memcpy( data, available_data, bytes_available );
        transfer_size -= bytes_available;
        data = ( (uint8_t*) data + bytes_available );
        ring_buffer_consume( uart_interfaces[uart].rx_buffer, bytes_available );
      } while ( transfer_size != 0 );
    }
    
    if ( size != 0 )
    {
         platform_log("uart receive 04"); 
      return kGeneralErr;
    }
    else
    {
         platform_log("uart receive 05"); 
      return kNoErr;
    }
  }
  else
  {
#endif 
    return platform_uart_receive_bytes( uart, data, size, timeout );
//  }
 // return kNoErr;
}


static OSStatus platform_uart_receive_bytes( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{

  /* Reset DMA transmission result. The result is assigned in interrupt handler */
  //  uart_interfaces[uart].rx_dma_result = kGeneralErr;
   //  if(UART_DRV_EdmaReceiveData(BOARD_DEBUG_UART_INSTANCE, data, size)==kStatus_UART_Success){
   if(UART_DRV_EdmaReceiveDataBlocking(BOARD_APP_UART_INSTANCE, data,1, timeout)==kStatus_UART_Success){
         platform_log("uart receive success.");
#if ADD_OS_CODE
        #ifndef NO_MICO_RTOS
            mico_rtos_set_semaphore( &uart_interfaces[uart].rx_complete );
        #else
            uart_interfaces[uart ].rx_complete = true;
        #endif
#endif
    }
    //if(UART_DRV_EdmaReceiveData(BOARD_DEBUG_UART_INSTANCE, data, size)==kStatus_UART_Success){}
  
#if ADD_OS_CODE
  if ( timeout > 0 )
  {
         platform_log("uart receive timeout.");
#ifndef NO_MICO_RTOS
    mico_rtos_get_semaphore( &uart_interfaces[uart].rx_complete, timeout );
#else
    uart_interfaces[uart].rx_complete = false;
    int delay_start = mico_get_time_no_os();
    while(uart_interfaces[uart].rx_complete == false){
      if(mico_get_time_no_os() >= delay_start + timeout && timeout != MICO_NEVER_TIMEOUT){
        break;
      }
    }    
#endif
    return uart_interfaces[uart].rx_dma_result;
  }   
#endif
  return kNoErr;
}


uint32_t MicoUartGetLengthInBuffer( mico_uart_t uart )
{
  return ring_buffer_used_space( uart_interfaces[uart].rx_buffer );
}

#ifndef NO_MICO_RTOS
static void thread_wakeup(void *arg)
{
  mico_uart_t uart = *(mico_uart_t *)arg;
  
  while(1){
//     if(mico_rtos_get_semaphore(&uart_interfaces[ uart ].sem_wakeup, 1000) != kNoErr){
//      gpio_irq_enable(uart_mapping[uart].pin_rx->bank, uart_mapping[uart].pin_rx->number, IRQ_TRIGGER_FALLING_EDGE, RX_PIN_WAKEUP_handler, &uart);
//      MicoMcuPowerSaveConfig(true);
//    }   
  }
}
#endif

/******************************************************
*            Interrupt Service Routines
******************************************************/
#ifndef NO_MICO_RTOS
void RX_PIN_WAKEUP_handler(void *arg)
{
  (void)arg;
  mico_uart_t uart = *(mico_uart_t *)arg;
  
//  RCC_AHB1PeriphClockCmd(uart_mapping[ uart ].pin_rx->peripheral_clock, ENABLE);
//  uart_mapping[ uart ].usart_peripheral_clock_func ( uart_mapping[uart].usart_peripheral_clock,  ENABLE );
//  uart_mapping[uart].rx_dma_peripheral_clock_func  ( uart_mapping[uart].rx_dma_peripheral_clock, ENABLE );
//  
//  gpio_irq_disable(uart_mapping[uart].pin_rx->bank, uart_mapping[uart].pin_rx->number); 
  
  MicoMcuPowerSaveConfig(false);
  mico_rtos_set_semaphore(&uart_interfaces[uart].sem_wakeup);
}
#endif

// end file






