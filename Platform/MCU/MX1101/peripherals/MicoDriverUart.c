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

#include "debug.h"
#include "MICORTOS.h"
#include "MICOPlatform.h"

#include "platform.h"
#include "platform_common_config.h"
#include "MicoDriverMapping.h"
#include "uart.h"
#include "gpio.h"
//#include "gpio_irq.h"

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

#ifndef NO_MICO_RTOS
static mico_uart_t current_uart;
#endif

/******************************************************
*               Function Declarations
******************************************************/

static OSStatus internal_uart_init ( mico_uart_t uart, const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer );

/* Interrupt service functions - called from interrupt vector table */
#ifndef NO_MICO_RTOS
static void thread_wakeup(void *arg);
static void RX_PIN_WAKEUP_handler(void *arg);
#endif

void usart2_rx_dma_irq( void );
void usart1_rx_dma_irq( void );
void usart6_rx_dma_irq( void );
void usart2_tx_dma_irq( void );
void usart1_tx_dma_irq( void );
void usart6_tx_dma_irq( void );
void usart2_irq       ( void );
void usart1_irq       ( void );
void usart6_irq       ( void );

/******************************************************
*               Function Definitions
******************************************************/

OSStatus MicoUartInitialize( mico_uart_t uart, const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer )
{
  // #ifndef MICO_DISABLE_STDIO
  //     if (uart == STDIO_UART)
  //     {
  //         return kGeneralErr;
  //     }
  // #endif
  
  return internal_uart_init(uart, config, optional_rx_buffer);
}


OSStatus MicoStdioUartInitialize( const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer )
{
  return internal_uart_init(STDIO_UART, config, optional_rx_buffer);
}


OSStatus internal_uart_init( mico_uart_t uart, const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer )
{
  OSStatus err = kNoErr;

  require_action(optional_rx_buffer!=NULL, exit, err = kParamErr);

#ifndef NO_MICO_RTOS
  mico_rtos_init_semaphore(&uart_interfaces[uart].tx_complete, 1);
  mico_rtos_init_semaphore(&uart_interfaces[uart].rx_complete, 1);
#else
  uart_interfaces[uart].tx_complete = false;
  uart_interfaces[uart].rx_complete = false;
#endif

  if(uart_mapping[uart].uart == FUART){
    if( uart_mapping[uart].pin_tx->port == GPIOA && uart_mapping[uart].pin_tx->pin == 1 )
      GpioFuartTxIoConfig(0);
    else if( uart_mapping[uart].pin_tx->port == GPIOB && uart_mapping[uart].pin_tx->pin == 7 )
      GpioFuartTxIoConfig(1);
    else if( uart_mapping[uart].pin_tx->port == GPIOC && uart_mapping[uart].pin_tx->pin == 3 )
      GpioFuartTxIoConfig(2);
    else
      return kUnsupportedErr;

    if( uart_mapping[uart].pin_rx->port == GPIOA && uart_mapping[uart].pin_rx->pin == 1 )
      GpioFuartRxIoConfig(0);
    else if( uart_mapping[uart].pin_rx->port == GPIOB && uart_mapping[uart].pin_rx->pin == 6 )
      GpioFuartRxIoConfig(1);
    else if( uart_mapping[uart].pin_rx->port == GPIOC && uart_mapping[uart].pin_rx->pin == 4 )
      GpioFuartRxIoConfig(2);
    else
      return kUnsupportedErr;

    require_action( config->flow_control == FLOW_CONTROL_DISABLED, exit, err = kUnsupportedErr );

    err = FuartInit(config->baud_rate, config->data_width + 5, config->parity, config->stop_bits + 1);
    require_noerr(err, exit);

    FuartIOctl(UART_IOCTL_RXINT_SET, 1);
    
    if (optional_rx_buffer != NULL){
      /* Note that the ring_buffer should've been initialised first */
      uart_interfaces[uart].rx_buffer = optional_rx_buffer;
      uart_interfaces[uart].rx_size   = 0;
      //platform_uart_receive_bytes( uart, optional_rx_buffer->buffer, optional_rx_buffer->size, 0 );
    }

  }else if(uart_mapping[uart].uart == BUART){
    if( uart_mapping[uart].pin_tx->port == GPIOA && uart_mapping[uart].pin_tx->pin == 16 )
      GpioBuartTxIoConfig(0);
    else if( uart_mapping[uart].pin_tx->port == GPIOA && uart_mapping[uart].pin_tx->pin == 25 )
      GpioBuartTxIoConfig(1);
    else if( uart_mapping[uart].pin_tx->port == GPIOB && uart_mapping[uart].pin_tx->pin == 9 )
      GpioBuartTxIoConfig(2);
    else if( uart_mapping[uart].pin_tx->port == GPIOB && uart_mapping[uart].pin_tx->pin == 28 )
      GpioBuartTxIoConfig(3);
    else
      return kUnsupportedErr;

    if( uart_mapping[uart].pin_rx->port == GPIOA && uart_mapping[uart].pin_rx->pin == 13 )
      GpioBuartRxIoConfig(0);
    else if( uart_mapping[uart].pin_rx->port == GPIOA && uart_mapping[uart].pin_rx->pin == 24 )
      GpioBuartRxIoConfig(1);
    else if( uart_mapping[uart].pin_rx->port == GPIOB && uart_mapping[uart].pin_rx->pin == 8 )
      GpioBuartRxIoConfig(2);
    else if( uart_mapping[uart].pin_rx->port == GPIOB && uart_mapping[uart].pin_rx->pin == 29 )
      GpioBuartRxIoConfig(3);
    else
      return kUnsupportedErr;

    require_action( config->flow_control == FLOW_CONTROL_DISABLED, exit, err = kUnsupportedErr );

    err =  BuartExFifoInit(0x5800, 1024*5, 1024*5, 1);
    require_noerr(err, exit);

    err = BuartInit(config->baud_rate, config->data_width + 5, config->parity, config->stop_bits + 1);
    require_noerr(err, exit);
    
    BuartIOctl(UART_IOCTL_RXINT_SET, 1);
    BuartIOctl(UART_IOCTL_TXINT_SET, 1);

  }else
    return kUnsupportedErr;

exit:
  return  err;
}

OSStatus MicoUartFinalize( mico_uart_t uart )
{
  return kNoErr;
}

OSStatus MicoUartSend( mico_uart_t uart, const void* data, uint32_t size )
{
  if(uart_mapping[uart].uart == FUART){
    FuartSend( (uint8_t *)data, size);
    return kNoErr;
  }else if(uart_mapping[uart].uart == BUART){
    BuartSend( (uint8_t *)data, size);
  }else
    return kUnsupportedErr;
 
#ifndef NO_MICO_RTOS
  mico_rtos_get_semaphore( &uart_interfaces[ uart ].tx_complete, MICO_NEVER_TIMEOUT );
#else 
  while(uart_interfaces[ uart ].tx_complete == false);
  uart_interfaces[ uart ].tx_complete = false;
#endif

}

OSStatus FUartRecv( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{

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
        
#ifndef NO_MICO_RTOS
        if ( mico_rtos_get_semaphore( &uart_interfaces[uart].rx_complete, timeout) != kNoErr )
        {
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
        
        /* Reset rx_size to prevent semaphore being set while nothing waits for the data */
        uart_interfaces[uart].rx_size = 0;
      }
      
      size -= transfer_size;
      
      // Grab data from the buffer
      do
      {
        uint8_t* available_data;
        uint32_t bytes_available;
        
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
      return kGeneralErr;
    }
    else
    {
      return kNoErr;
    }
  }
  else
  {
#ifndef NO_MICO_RTOS
    mico_thread_msleep(timeout);
#else
    mico_thread_msleep_no_os(timeout);
#endif

    return kNoMemoryErr;
  }
}

OSStatus BUartRecv( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{


    while (size != 0)
    {
      uint32_t transfer_size = MIN(uart_interfaces[uart].rx_buffer->size / 2, size);
      
      /* Check if ring buffer already contains the required amount of data. */
      if ( transfer_size > (uint32_t)BuartIOctl(BUART_IOCTL_RXFIFO_DATLEN_GET, 0) )
      {
        /* Set rx_size and wait in rx_complete semaphore until data reaches rx_size or timeout occurs */
        uart_interfaces[uart].rx_size = transfer_size;
        
        BuartIOctl(BUART_IOCTL_RXFIFO_TRGR_DEPTH_SET, uart_interfaces[uart].rx_size-1);
        BuartIOctl(UART_IOCTL_RXINT_SET, 1);
        
#ifndef NO_MICO_RTOS
        if ( mico_rtos_get_semaphore( &uart_interfaces[uart].rx_complete, timeout) != kNoErr )
        {
          //BuartIOctl(UART_IOCTL_RXINT_SET, 0);
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
        
        /* Reset rx_size to prevent semaphore being set while nothing waits for the data */
        uart_interfaces[uart].rx_size = 0;
      }
      
      size -= transfer_size;
      
      // Grab data from the buffer
      do
      {
        uint32_t bytes_available;

        bytes_available = (uint32_t)BuartIOctl(BUART_IOCTL_RXFIFO_DATLEN_GET, 0);
        bytes_available = MIN( bytes_available, transfer_size );
        BuartRecv(data, bytes_available, 0);
        transfer_size -= bytes_available;
        data = ( (uint8_t*) data + bytes_available );
      } while ( transfer_size != 0 );
    }
    
    if ( size != 0 )
    {
      return kGeneralErr;
    }
    else
    {
      return kNoErr;
    }
}

OSStatus MicoUartRecv( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{
  if(uart_mapping[uart].uart == FUART)
    FUartRecv( uart, data, size, timeout );
  else if(uart_mapping[uart].uart == BUART)
    BUartRecv( uart, data, size, timeout );
}

uint32_t MicoUartGetLengthInBuffer( mico_uart_t uart )
{
  uint32_t buart_rx_fifo_data_len = 0;
  if(uart_mapping[uart].uart == FUART)
    return ring_buffer_used_space( uart_interfaces[uart].rx_buffer );
  else if(uart_mapping[uart].uart == BUART){
    return BuartIOctl(BUART_IOCTL_RXFIFO_DATLEN_GET, 0);
  }
}

#ifndef NO_MICO_RTOS
static void thread_wakeup(void *arg)
{
}
#endif

/******************************************************
*            Interrupt Service Routines
******************************************************/
#ifndef NO_MICO_RTOS
void RX_PIN_WAKEUP_handler(void *arg)
{

}
#endif

void FuartInterrupt(void)
{
  int status;
  uint8_t rxData;
  status = FuartIOctl(UART_IOCTL_RXSTAT_GET,0);

  if(status & 0x1E){
    /*
     * clear FIFO before clear other flags
     */
    FuartIOctl(UART_IOCTL_RXFIFO_CLR,0);
    /*
     * clear other error flags
     */
    FuartIOctl(UART_IOCTL_RXINT_CLR,0);
  }

  if(status & 0x01)
  { 
    //or,you can receive them in the interrupt directly
    while(FuartRecvByte(&rxData) > 0){
      ring_buffer_write( uart_interfaces[ AP80xx_FUART ].rx_buffer, &rxData,1 );
    }

    FuartIOctl(UART_IOCTL_RXINT_CLR,0);

    // Notify thread if sufficient data are available
    if ( ( uart_interfaces[ 0 ].rx_size > 0 ) &&
        ( ring_buffer_used_space( uart_interfaces[ AP80xx_FUART ].rx_buffer ) >= uart_interfaces[ AP80xx_FUART ].rx_size ) )
    {
  #ifndef NO_MICO_RTOS
      mico_rtos_set_semaphore( &uart_interfaces[ AP80xx_FUART ].rx_complete );
  #else
      uart_interfaces[ AP80xx_FUART ].rx_complete = true;
  #endif
      uart_interfaces[ AP80xx_FUART ].rx_size = 0;
    }
  }

  if(FuartIOctl(UART_IOCTL_TXSTAT_GET,0) & 0x01)
  {
    FuartIOctl(UART_IOCTL_TXINT_CLR,0);
#ifndef NO_MICO_RTOS
    mico_rtos_set_semaphore( &uart_interfaces[ AP80xx_FUART ].tx_complete );
#else
    uart_interfaces[ AP80xx_FUART ].tx_complete = true;
#endif
  }
}

void BuartInterrupt(void)
{
  int status;
  uint8_t rxData;
  status = BuartIOctl(UART_IOCTL_RXSTAT_GET,0);

  if(status & 0x1E){
    /*
     * clear FIFO before clear other flags
     */
    BuartIOctl(UART_IOCTL_RXFIFO_CLR,0);
    /*
     * clear other error flags
     */
    BuartIOctl(UART_IOCTL_RXINT_CLR,0);
  }

  if(status & 0x01)
  { 
    BuartIOctl(UART_IOCTL_RXINT_SET, 0);
    BuartIOctl(UART_IOCTL_RXINT_CLR,0);

  // Notify thread if sufficient data are available
    if ( ( uart_interfaces[ AP80xx_BUART ].rx_size > 0 ) &&
        ( (uint32_t)BuartIOctl(BUART_IOCTL_RXFIFO_DATLEN_GET, 0)  >= uart_interfaces[ AP80xx_BUART ].rx_size ) )
    {
  #ifndef NO_MICO_RTOS
      mico_rtos_set_semaphore( &uart_interfaces[ AP80xx_BUART ].rx_complete );
  #else
      uart_interfaces[ AP80xx_BUART ].rx_complete = true;
  #endif
      uart_interfaces[ AP80xx_BUART ].rx_size = 0;
    }
  }
  
// #ifndef NO_MICO_RTOS
//   if(uart_interfaces[ 0 ].sem_wakeup)
//     mico_rtos_set_semaphore(&uart_interfaces[ 0 ].sem_wakeup);
// #endif

  if(BuartIOctl(UART_IOCTL_TXSTAT_GET,0) & 0x01)
  {
    BuartIOctl(UART_IOCTL_TXINT_CLR,0);

#ifndef NO_MICO_RTOS
    mico_rtos_set_semaphore( &uart_interfaces[ AP80xx_BUART ].tx_complete );
#else
    uart_interfaces[ AP80xx_BUART ].tx_complete = true;
#endif
  }
}



