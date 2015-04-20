/**
******************************************************************************
* @file    mico_driver_uart.c 
* @author  william xu
* @version V1.0.0
* @date    05-may-2014
* @brief   this file provide UART driver functions.
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


#include "mico_rtos.h"
#include "mico_platform.h"

#include "platform.h"

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
*               variables definitions
******************************************************/

static uart_interface_t uart_interfaces[NUMBER_OF_UART_INTERFACES];

#ifndef NO_MICO_RTOS
static mico_uart_t current_uart;
#endif

/******************************************************
*               function declarations
******************************************************/

static OSStatus internal_uart_init ( mico_uart_t uart, const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer );
static OSStatus platform_uart_receive_bytes( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout );




/* interrupt service functions - called from interrupt vector table */
#ifndef NO_MICO_RTOS
static void thread_wakeup(void *arg);
static void rx_pin_wakeup_handler(void *arg);
#endif

void usart2_rx_dma_irq( void );
void usart1_rx_dma_irq( void );

/******************************************************
*               function definitions
******************************************************/

OSStatus mico_uart_initialize( mico_uart_t uart, const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer )
{
  // #ifndef MICO_DISABLE_STDIO
  //     if (uart == STDIO_UART)
  //     {
  //         return kGeneralErr;
  //     }
  // #endif
  
  return internal_uart_init(uart, config, optional_rx_buffer);
}


OSStatus mico_stdio_uart_initialize( const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer )
{
  return internal_uart_init(STDIO_UART, config, optional_rx_buffer);
}


OSStatus internal_uart_init( mico_uart_t uart, const mico_uart_config_t* config, ring_buffer_t* optional_rx_buffer )
{
  
#ifndef NO_MICO_RTOS
  mico_rtos_init_semaphore(&uart_interfaces[uart].tx_complete, 1);
  mico_rtos_init_semaphore(&uart_interfaces[uart].rx_complete, 1);
#else
  uart_interfaces[uart].tx_complete = false;
  uart_interfaces[uart].rx_complete = false;
#endif
  
  mico_mcu_power_save_config(false);
#ifndef NO_MICO_RTOS
  if(config->flags & UART_WAKEUP_ENABLE){
    current_uart = uart;
    mico_rtos_init_semaphore( &uart_interfaces[uart].sem_wakeup, 1 );
    mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART_WAKEUP", thread_wakeup, 0x100, &current_uart);
  }
#endif
  /* setup ring buffer */
  if (optional_rx_buffer != NULL)
  {
    /* note that the ring_buffer should've been initialised first */
    uart_interfaces[uart].rx_buffer = optional_rx_buffer;
    uart_interfaces[uart].rx_size   = 0;
    platform_uart_receive_bytes( uart, optional_rx_buffer->buffer, optional_rx_buffer->size, 0 );
  }
  else
  {
  }
  
  mico_mcu_power_save_config(true);
  
  return kNoErr;
}

OSStatus mico_uart_finalize( mico_uart_t uart )
{
  
  mico_mcu_power_save_config(false);
 
#ifndef NO_MICO_RTOS
  mico_rtos_deinit_semaphore(&uart_interfaces[uart].rx_complete);
  mico_rtos_deinit_semaphore(&uart_interfaces[uart].tx_complete);
#endif
  
  mico_mcu_power_save_config(true);
  
  return kNoErr;
}

OSStatus mico_uart_send( mico_uart_t uart, const void* data, uint32_t size )
{
  /* reset DMA transmission result. the result is assigned in interrupt handler */
  uart_interfaces[uart].tx_dma_result = kGeneralErr;
  
  mico_mcu_power_save_config(false);  
  
#ifndef NO_MICO_RTOS
  mico_rtos_get_semaphore( &uart_interfaces[ uart ].tx_complete, MICO_NEVER_TIMEOUT );
#else 
  while(uart_interfaces[ uart ].tx_complete == false);
  uart_interfaces[ uart ].tx_complete = false;
#endif
  
  mico_mcu_power_save_config(true);
  
  return uart_interfaces[uart].tx_dma_result;
  //#endif
}

OSStatus mico_uart_recv( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{
  if (uart_interfaces[uart].rx_buffer != NULL)
  {
    while (size != 0)
    {
      uint32_t transfer_size = MIN(uart_interfaces[uart].rx_buffer->size / 2, size);
      
      /* check if ring buffer already contains the required amount of data. */
      if ( transfer_size > ring_buffer_used_space( uart_interfaces[uart].rx_buffer ) )
      {
        /* set rx_size and wait in rx_complete semaphore until data reaches rx_size or timeout occurs */
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
        
        /* reset rx_size to prevent semaphore being set while nothing waits for the data */
        uart_interfaces[uart].rx_size = 0;
      }
      
      size -= transfer_size;
      
      // grab data from the buffer
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
    return platform_uart_receive_bytes( uart, data, size, timeout );
  }
}

static OSStatus platform_uart_receive_bytes( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{
  if ( uart_interfaces[uart].rx_buffer != NULL )
  {
  }
  else
  {
  }
  
  /* reset DMA transmission result. the result is assigned in interrupt handler */
  uart_interfaces[uart].rx_dma_result = kGeneralErr;
  
  
  
  if ( timeout > 0 )
  {
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
  
  
  return kNoErr;
}

uint32_t mico_uart_get_length_in_buffer( mico_uart_t uart )
{
  return ring_buffer_used_space( uart_interfaces[uart].rx_buffer );
}

#ifndef NO_MICO_RTOS
static void thread_wakeup(void *arg)
{
  mico_uart_t uart = *(mico_uart_t *)arg;
  
  while(1){
    if(mico_rtos_get_semaphore(&uart_interfaces[ uart ].sem_wakeup, 1000) != kNoErr){
    mico_mcu_power_save_config(true);
    }
  }
}
#endif

/******************************************************
*            interrupt service routines
******************************************************/
#ifndef NO_MICO_RTOS
void rx_pin_wakeup_handler(void *arg)
{
  (void)arg;
  mico_uart_t uart = *(mico_uart_t *)arg;
  mico_mcu_power_save_config(false);
  mico_rtos_set_semaphore(&uart_interfaces[uart].sem_wakeup);
}
#endif


void USART1_IRQHandler( void )
{
  // clear all inte
}

void USART2_IRQHandler( void )
{
  // clear all interrupts. i
}


