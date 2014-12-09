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
#include "stm32f2xx_platform.h"
#include "stm32f2xx.h"
#include "gpio_irq.h"

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
static OSStatus platform_uart_receive_bytes( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout );




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
  GPIO_InitTypeDef  gpio_init_structure;
  USART_InitTypeDef usart_init_structure;
  NVIC_InitTypeDef  nvic_init_structure;
  DMA_InitTypeDef   dma_init_structure;
  
#ifndef NO_MICO_RTOS
  mico_rtos_init_semaphore(&uart_interfaces[uart].tx_complete, 1);
  mico_rtos_init_semaphore(&uart_interfaces[uart].rx_complete, 1);
#else
  uart_interfaces[uart].tx_complete = false;
  uart_interfaces[uart].rx_complete = false;
#endif
  
  MicoMcuPowerSaveConfig(false);
  
  /* Enable GPIO peripheral clocks for TX and RX pins */
  RCC_AHB1PeriphClockCmd( uart_mapping[uart].pin_rx->peripheral_clock |
                         uart_mapping[uart].pin_tx->peripheral_clock, ENABLE );
  
  /* Configure USART TX Pin */
  gpio_init_structure.GPIO_Pin   = (uint32_t) ( 1 << uart_mapping[uart].pin_tx->number );
  gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF;
  gpio_init_structure.GPIO_OType = GPIO_OType_PP;
  gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init( uart_mapping[uart].pin_tx->bank, &gpio_init_structure );
  GPIO_PinAFConfig( uart_mapping[uart].pin_tx->bank, uart_mapping[uart].pin_tx->number, uart_mapping[uart].gpio_af );
  
  /* Configure USART RX Pin */
  gpio_init_structure.GPIO_Pin   = (uint32_t) ( 1 << uart_mapping[uart].pin_rx->number );
  gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF;
  gpio_init_structure.GPIO_OType = GPIO_OType_OD;
  gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init( uart_mapping[uart].pin_rx->bank, &gpio_init_structure );
  GPIO_PinAFConfig( uart_mapping[uart].pin_rx->bank, uart_mapping[uart].pin_rx->number, uart_mapping[uart].gpio_af );
  
#ifndef NO_MICO_RTOS
  if(config->flags & UART_WAKEUP_ENABLE){
    current_uart = uart;
    mico_rtos_init_semaphore( &uart_interfaces[uart].sem_wakeup, 1 );
    mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART_WAKEUP", thread_wakeup, 0x100, &current_uart);
  }
#endif
  
  /* Check if any of the flow control is enabled */
  if ( uart_mapping[uart].pin_cts && (config->flow_control == FLOW_CONTROL_CTS || config->flow_control == FLOW_CONTROL_CTS_RTS) )
  {
    /* Enable peripheral clock */
    RCC_AHB1PeriphClockCmd( uart_mapping[uart].pin_cts->peripheral_clock, ENABLE );
    
    /* Configure CTS Pin */
    gpio_init_structure.GPIO_Pin   = (uint32_t) ( 1 << uart_mapping[uart].pin_cts->number );
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF;
    gpio_init_structure.GPIO_OType = GPIO_OType_OD;
    gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init( uart_mapping[uart].pin_cts->bank, &gpio_init_structure );
    GPIO_PinAFConfig( uart_mapping[uart].pin_cts->bank, uart_mapping[uart].pin_cts->number, uart_mapping[uart].gpio_af );
  }
  
  if ( uart_mapping[uart].pin_cts && (config->flow_control == FLOW_CONTROL_RTS || config->flow_control == FLOW_CONTROL_CTS_RTS) )
  {
    /* Enable peripheral clock */
    RCC_AHB1PeriphClockCmd( uart_mapping[uart].pin_rts->peripheral_clock, ENABLE );
    
    /* Configure RTS Pin */
    gpio_init_structure.GPIO_Pin   = (uint32_t) ( 1 << uart_mapping[uart].pin_rts->number );
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF;
    gpio_init_structure.GPIO_OType = GPIO_OType_OD;
    gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init( uart_mapping[uart].pin_rts->bank, &gpio_init_structure );
    GPIO_PinAFConfig( uart_mapping[uart].pin_rts->bank, uart_mapping[uart].pin_rts->number, uart_mapping[uart].gpio_af );
  }
  
  /* Enable UART peripheral clock */
  uart_mapping[uart].usart_peripheral_clock_func( uart_mapping[uart].usart_peripheral_clock, ENABLE );
  
  /**************************************************************************
  * Initialise STM32 USART registers
  * NOTE:
  * - Both transmitter and receiver are disabled until usart_enable_transmitter/receiver is called.
  * - Only 1 and 2 stop bits are implemented at the moment.
  **************************************************************************/
  usart_init_structure.USART_Mode       = 0;
  usart_init_structure.USART_BaudRate   = config->baud_rate;
  usart_init_structure.USART_WordLength = ( ( config->data_width == DATA_WIDTH_9BIT ) ||
                                           ( ( config->data_width == DATA_WIDTH_8BIT ) && ( config->parity != NO_PARITY ) ) ) ? USART_WordLength_9b : USART_WordLength_8b;
  usart_init_structure.USART_StopBits   = ( config->stop_bits == STOP_BITS_1 ) ? USART_StopBits_1 : USART_StopBits_2;
                                           
  switch ( config->parity )
  {
  case NO_PARITY:
    usart_init_structure.USART_Parity = USART_Parity_No;
    break;
  case EVEN_PARITY:
    usart_init_structure.USART_Parity = USART_Parity_Even;
    break;
  case ODD_PARITY:
    usart_init_structure.USART_Parity = USART_Parity_Odd;
    break;
  default:
    return kParamErr;
  }
  
  switch ( config->flow_control )
  {
  case FLOW_CONTROL_DISABLED:
    usart_init_structure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    break;
  case FLOW_CONTROL_CTS:
    usart_init_structure.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS;
    break;
  case FLOW_CONTROL_RTS:
    usart_init_structure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS;
    break;
  case FLOW_CONTROL_CTS_RTS:
    usart_init_structure.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
    break;
  default:
    return kParamErr;
  }
  
  /* Initialise USART peripheral */
  USART_Init( uart_mapping[uart].usart, &usart_init_structure );
  
  
  /**************************************************************************
  * Initialise STM32 DMA registers
  * Note: If DMA is used, USART interrupt isn't enabled.
  **************************************************************************/
  /* Enable DMA peripheral clock */
  uart_mapping[uart].tx_dma_peripheral_clock_func( uart_mapping[uart].tx_dma_peripheral_clock, ENABLE );
  uart_mapping[uart].rx_dma_peripheral_clock_func( uart_mapping[uart].rx_dma_peripheral_clock, ENABLE );
  
  /* Fill init structure with common DMA settings */
  dma_init_structure.DMA_PeripheralInc   = DMA_PeripheralInc_Disable;
  dma_init_structure.DMA_MemoryInc       = DMA_MemoryInc_Enable;
  dma_init_structure.DMA_Priority        = DMA_Priority_VeryHigh;
  dma_init_structure.DMA_FIFOMode        = DMA_FIFOMode_Disable;
  dma_init_structure.DMA_FIFOThreshold   = DMA_FIFOThreshold_Full;
  dma_init_structure.DMA_MemoryBurst     = DMA_MemoryBurst_Single;
  dma_init_structure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  
  if ( config->data_width == DATA_WIDTH_9BIT )
  {
    dma_init_structure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dma_init_structure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
  }
  else
  {
    dma_init_structure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_init_structure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
  }
  
  /* Initialise TX DMA */
  DMA_DeInit( uart_mapping[uart].tx_dma_stream );
  dma_init_structure.DMA_Channel            = uart_mapping[uart].tx_dma_channel;
  dma_init_structure.DMA_PeripheralBaseAddr = (uint32_t) &uart_mapping[uart].usart->DR;
  dma_init_structure.DMA_Memory0BaseAddr    = (uint32_t) 0;
  dma_init_structure.DMA_DIR                = DMA_DIR_MemoryToPeripheral;
  dma_init_structure.DMA_BufferSize         = 0;
  dma_init_structure.DMA_Mode               = DMA_Mode_Normal;
  DMA_Init( uart_mapping[uart].tx_dma_stream, &dma_init_structure );
  
  /* Initialise RX DMA */
  DMA_DeInit( uart_mapping[uart].rx_dma_stream );
  dma_init_structure.DMA_Channel            = uart_mapping[uart].rx_dma_channel;
  dma_init_structure.DMA_PeripheralBaseAddr = (uint32_t) &uart_mapping[uart].usart->DR;
  dma_init_structure.DMA_Memory0BaseAddr    = 0;
  dma_init_structure.DMA_DIR                = DMA_DIR_PeripheralToMemory;
  dma_init_structure.DMA_BufferSize         = 0;
  dma_init_structure.DMA_Mode               = DMA_Mode_Normal;
  DMA_Init( uart_mapping[uart].rx_dma_stream, &dma_init_structure );
  
  /**************************************************************************
  * Initialise STM32 DMA interrupts
  * Note: Only TX DMA interrupt is enabled.
  **************************************************************************/
  
  /* Configure TX DMA interrupt on Cortex-M3 */
  nvic_init_structure.NVIC_IRQChannel                   = uart_mapping[uart].tx_dma_irq;
  nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0x5;
  nvic_init_structure.NVIC_IRQChannelSubPriority        = 0x8;
  nvic_init_structure.NVIC_IRQChannelCmd                = ENABLE;
  NVIC_Init( &nvic_init_structure );
  
  /* Enable TC (transfer complete) and TE (transfer error) interrupts on source */
  DMA_ITConfig( uart_mapping[uart].tx_dma_stream, DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_FE, ENABLE );
  
  /* Enable USART's RX DMA interfaces */
  USART_DMACmd( uart_mapping[uart].usart, USART_DMAReq_Rx, ENABLE );
  
  /**************************************************************************
  * Initialise STM32 USART interrupt
  **************************************************************************/
  nvic_init_structure.NVIC_IRQChannel                   = uart_mapping[uart].usart_irq;
  nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0x6;
  nvic_init_structure.NVIC_IRQChannelSubPriority        = 0x7;
  nvic_init_structure.NVIC_IRQChannelCmd                = ENABLE;
  
  /* Enable USART interrupt vector in Cortex-M3 */
  NVIC_Init( &nvic_init_structure );
  
  /* Enable USART */
  USART_Cmd( uart_mapping[uart].usart, ENABLE );
  
  /* Enable both transmit and receive */
  uart_mapping[uart].usart->CR1 |= USART_CR1_TE;
  uart_mapping[uart].usart->CR1 |= USART_CR1_RE;
  
  /* Setup ring buffer */
  if (optional_rx_buffer != NULL)
  {
    /* Note that the ring_buffer should've been initialised first */
    uart_interfaces[uart].rx_buffer = optional_rx_buffer;
    uart_interfaces[uart].rx_size   = 0;
    platform_uart_receive_bytes( uart, optional_rx_buffer->buffer, optional_rx_buffer->size, 0 );
  }
  else
  {
    /* Not using ring buffer. Configure RX DMA interrupt on Cortex-M3 */
    nvic_init_structure.NVIC_IRQChannel                   = uart_mapping[uart].rx_dma_irq;
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0x5;
    nvic_init_structure.NVIC_IRQChannelSubPriority        = 0x8;
    nvic_init_structure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init( &nvic_init_structure );
    
    /* Enable TC (transfer complete) and TE (transfer error) interrupts on source */
    DMA_ITConfig( uart_mapping[uart].rx_dma_stream, DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_FE, ENABLE );
  }
  
  MicoMcuPowerSaveConfig(true);
  
  return kNoErr;
}

OSStatus MicoUartFinalize( mico_uart_t uart )
{
  NVIC_InitTypeDef nvic_init_structure;
  
  MicoMcuPowerSaveConfig(false);
  
  /* Disable USART */
  USART_Cmd( uart_mapping[uart].usart, DISABLE );
  
  /* Deinitialise USART */
  USART_DeInit( uart_mapping[uart].usart );
  
  /**************************************************************************
  * De-initialise STM32 DMA and interrupt
  **************************************************************************/
  
  /* Deinitialise DMA streams */
  DMA_DeInit( uart_mapping[uart].tx_dma_stream );
  DMA_DeInit( uart_mapping[uart].rx_dma_stream );
  
  /* Disable TC (transfer complete) interrupt at the source */
  DMA_ITConfig( uart_mapping[uart].tx_dma_stream, DMA_IT_TC | DMA_IT_TE, DISABLE );
  
  /* Disable transmit DMA interrupt at Cortex-M3 */
  nvic_init_structure.NVIC_IRQChannel                   = uart_mapping[uart].tx_dma_irq;
  nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0x5;
  nvic_init_structure.NVIC_IRQChannelSubPriority        = 0x8;
  nvic_init_structure.NVIC_IRQChannelCmd                = DISABLE;
  NVIC_Init( &nvic_init_structure );
  
  /* Disable DMA peripheral clocks */
  uart_mapping[uart].tx_dma_peripheral_clock_func( uart_mapping[uart].tx_dma_peripheral_clock, DISABLE );
  uart_mapping[uart].rx_dma_peripheral_clock_func( uart_mapping[uart].rx_dma_peripheral_clock, DISABLE );
  
  /**************************************************************************
  * De-initialise STM32 USART interrupt
  **************************************************************************/
  
  USART_ITConfig( uart_mapping[uart].usart, USART_IT_RXNE, DISABLE );
  
  /* Disable UART interrupt vector on Cortex-M3 */
  nvic_init_structure.NVIC_IRQChannel                   = uart_mapping[uart].usart_irq;
  nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0xf;
  nvic_init_structure.NVIC_IRQChannelSubPriority        = 0xf;
  nvic_init_structure.NVIC_IRQChannelCmd                = DISABLE;
  NVIC_Init( &nvic_init_structure );
  
  /* Disable registers clocks */
  uart_mapping[uart].usart_peripheral_clock_func( uart_mapping[uart].usart_peripheral_clock, DISABLE );
  
#ifndef NO_MICO_RTOS
  mico_rtos_deinit_semaphore(&uart_interfaces[uart].rx_complete);
  mico_rtos_deinit_semaphore(&uart_interfaces[uart].tx_complete);
#endif
  
  MicoMcuPowerSaveConfig(true);
  
  return kNoErr;
}

OSStatus MicoUartSend( mico_uart_t uart, const void* data, uint32_t size )
{
  /* Reset DMA transmission result. The result is assigned in interrupt handler */
  uart_interfaces[uart].tx_dma_result = kGeneralErr;
  
  MicoMcuPowerSaveConfig(false);  
  
  uart_mapping[uart].tx_dma_stream->CR  &= ~(uint32_t) DMA_SxCR_CIRC;
  uart_mapping[uart].tx_dma_stream->NDTR = size;
  uart_mapping[uart].tx_dma_stream->M0AR = (uint32_t)data;
  
  USART_DMACmd( uart_mapping[uart].usart, USART_DMAReq_Tx, ENABLE );
  USART_ClearFlag( uart_mapping[uart].usart, USART_FLAG_TC );
  DMA_Cmd( uart_mapping[uart].tx_dma_stream, ENABLE );
  
#ifndef NO_MICO_RTOS
  mico_rtos_get_semaphore( &uart_interfaces[ uart ].tx_complete, MICO_NEVER_TIMEOUT );
#else 
  while(uart_interfaces[ uart ].tx_complete == false);
  uart_interfaces[ uart ].tx_complete = false;
#endif
  
  while ( ( uart_mapping[ uart ].usart->SR & USART_SR_TC ) == 0 )
  {
  }
  
  DMA_Cmd( uart_mapping[uart].tx_dma_stream, DISABLE );
  USART_DMACmd( uart_mapping[uart].usart, USART_DMAReq_Tx, DISABLE );
  
  MicoMcuPowerSaveConfig(true);
  
  return uart_interfaces[uart].tx_dma_result;
  //#endif
}

OSStatus MicoUartRecv( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout )
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
    return platform_uart_receive_bytes( uart, data, size, timeout );
  }
}

static OSStatus platform_uart_receive_bytes( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{
  if ( uart_interfaces[uart].rx_buffer != NULL )
  {
    uart_mapping[uart].rx_dma_stream->CR |= DMA_SxCR_CIRC;
    
    // Enabled individual byte interrupts so progress can be updated
    USART_ITConfig( uart_mapping[uart].usart, USART_IT_RXNE, ENABLE );
  }
  else
  {
    uart_mapping[uart].rx_dma_stream->CR &= ~(uint32_t) DMA_SxCR_CIRC;
  }
  
  /* Reset DMA transmission result. The result is assigned in interrupt handler */
  uart_interfaces[uart].rx_dma_result = kGeneralErr;
  
  uart_mapping[uart].rx_dma_stream->NDTR = size;
  uart_mapping[uart].rx_dma_stream->M0AR = (uint32_t)data;
  uart_mapping[uart].rx_dma_stream->CR  |= DMA_SxCR_EN;
  
  
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

uint32_t MicoUartGetLengthInBuffer( mico_uart_t uart )
{
  return ring_buffer_used_space( uart_interfaces[uart].rx_buffer );
}

#ifndef NO_MICO_RTOS
static void thread_wakeup(void *arg)
{
  mico_uart_t uart = *(mico_uart_t *)arg;
  
  while(1){
    if(mico_rtos_get_semaphore(&uart_interfaces[ uart ].sem_wakeup, 1000) != kNoErr){
      gpio_irq_enable(uart_mapping[uart].pin_rx->bank, uart_mapping[uart].pin_rx->number, IRQ_TRIGGER_FALLING_EDGE, RX_PIN_WAKEUP_handler, &uart);
      MicoMcuPowerSaveConfig(true);
    }
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
  
  RCC_AHB1PeriphClockCmd(uart_mapping[ uart ].pin_rx->peripheral_clock, ENABLE);
  uart_mapping[ uart ].usart_peripheral_clock_func ( uart_mapping[uart].usart_peripheral_clock,  ENABLE );
  uart_mapping[uart].rx_dma_peripheral_clock_func  ( uart_mapping[uart].rx_dma_peripheral_clock, ENABLE );
  
  gpio_irq_disable(uart_mapping[uart].pin_rx->bank, uart_mapping[uart].pin_rx->number);
  MicoMcuPowerSaveConfig(false);
  mico_rtos_set_semaphore(&uart_interfaces[uart].sem_wakeup);
}
#endif


void USART1_IRQHandler( void )
{
  // Clear all interrupts. It's safe to do so because only RXNE interrupt is enabled
  USART1->SR = (uint16_t) (USART1->SR | 0xffff);
  
  // Update tail
  uart_interfaces[ STM32_UART_1 ].rx_buffer->tail = uart_interfaces[ STM32_UART_1 ].rx_buffer->size - uart_mapping[ STM32_UART_1 ].rx_dma_stream->NDTR;
  
  // Notify thread if sufficient data are available
  if ( ( uart_interfaces[ STM32_UART_1 ].rx_size > 0 ) &&
      ( ring_buffer_used_space( uart_interfaces[ STM32_UART_1 ].rx_buffer ) >= uart_interfaces[STM32_UART_1].rx_size ) )
  {
#ifndef NO_MICO_RTOS
    mico_rtos_set_semaphore( &uart_interfaces[ STM32_UART_1 ].rx_complete );
#else
    uart_interfaces[ STM32_UART_1 ].rx_complete = true;
#endif
    uart_interfaces[ STM32_UART_1 ].rx_size = 0;
  }
  
#ifndef NO_MICO_RTOS
  if(uart_interfaces[ STM32_UART_1 ].sem_wakeup)
    mico_rtos_set_semaphore(&uart_interfaces[ STM32_UART_1 ].sem_wakeup);
#endif
}

void USART2_IRQHandler( void )
{
  // Clear all interrupts. It's safe to do so because only RXNE interrupt is enabled
  USART2->SR = (uint16_t) (USART2->SR | 0xffff);
  
  // Update tail
  uart_interfaces[ STM32_UART_2 ].rx_buffer->tail = uart_interfaces[ STM32_UART_2 ].rx_buffer->size - uart_mapping[ STM32_UART_2 ].rx_dma_stream->NDTR;
  
  // Notify thread if sufficient data are available
  if ( ( uart_interfaces[ STM32_UART_2 ].rx_size > 0 ) &&
      ( ring_buffer_used_space( uart_interfaces[ STM32_UART_2 ].rx_buffer ) >= uart_interfaces[STM32_UART_2].rx_size ) )
  {
#ifndef NO_MICO_RTOS
    mico_rtos_set_semaphore( &uart_interfaces[ STM32_UART_2 ].rx_complete );
#else
    uart_interfaces[ STM32_UART_2 ].rx_complete = true;
#endif
    uart_interfaces[ STM32_UART_2 ].rx_size = 0;
  }
  
#ifndef NO_MICO_RTOS
  if(uart_interfaces[ STM32_UART_2 ].sem_wakeup)
    mico_rtos_set_semaphore(&uart_interfaces[ STM32_UART_2 ].sem_wakeup);
#endif
}

void USART6_IRQHandler( void )
{
  // Clear all interrupts. It's safe to do so because only RXNE interrupt is enabled
  USART6->SR = (uint16_t) (USART6->SR | 0xffff);
  
  // Update tail
  uart_interfaces[ STM32_UART_6 ].rx_buffer->tail = uart_interfaces[ STM32_UART_6 ].rx_buffer->size - uart_mapping[ STM32_UART_6 ].rx_dma_stream->NDTR;
  
  // Notify thread if sufficient data are available
  if ( ( uart_interfaces[ STM32_UART_6 ].rx_size > 0 ) &&
      ( ring_buffer_used_space( uart_interfaces[ STM32_UART_6 ].rx_buffer ) >= uart_interfaces[ STM32_UART_6 ].rx_size ) )
  {
#ifndef NO_MICO_RTOS
    mico_rtos_set_semaphore( &uart_interfaces[ STM32_UART_6 ].rx_complete );
#else
    uart_interfaces[ STM32_UART_1 ].rx_complete = true;
#endif
    uart_interfaces[ STM32_UART_6 ].rx_size = 0;
  }
  
#ifndef NO_MICO_RTOS
  if(uart_interfaces[ STM32_UART_6 ].sem_wakeup)
    mico_rtos_set_semaphore(&uart_interfaces[ STM32_UART_6 ].sem_wakeup);
#endif
}

//usart1_tx_dma_irq
void DMA2_Stream7_IRQHandler( void )
{
  bool tx_complete = false;
  
  if ( ( DMA2->HISR & DMA_HISR_TCIF7 ) != 0 )
  {
    /* Clear interrupt */
    DMA2->HIFCR |= DMA_HISR_TCIF7;
    uart_interfaces[ STM32_UART_1 ].tx_dma_result = kNoErr;
    tx_complete = true;
  }
  
  /* TX DMA error */
  if ( ( DMA2->HISR & ( DMA_HISR_TEIF7 | DMA_HISR_DMEIF7 | DMA_HISR_FEIF7 ) ) != 0 )
  {
    /* Clear interrupt */
    DMA2->HIFCR |= ( DMA_HISR_TEIF7 | DMA_HISR_DMEIF7 | DMA_HISR_FEIF7 );
    
    if ( tx_complete == false )
    {
      uart_interfaces[ STM32_UART_1 ].tx_dma_result = kGeneralErr;
    }
  }
  
#ifndef NO_MICO_RTOS
  /* Set semaphore regardless of result to prevent waiting thread from locking up */
  mico_rtos_set_semaphore( &uart_interfaces[ STM32_UART_1 ].tx_complete);
#else
  uart_interfaces[ STM32_UART_1 ].tx_complete = true;
#endif
}

//usart2_tx_dma_irq
void DMA1_Stream6_IRQHandler( void )
{
  bool tx_complete = false;
  
  if ( ( DMA1->HISR & DMA_HISR_TCIF6 ) != 0 )
  {
    /* Clear interrupt */
    DMA1->HIFCR |= DMA_HISR_TCIF6;
    uart_interfaces[ STM32_UART_2 ].tx_dma_result = kNoErr;
    tx_complete = true;
  }
  
  /* TX DMA error */
  if ( ( DMA1->HISR & ( DMA_HISR_TEIF6 | DMA_HISR_DMEIF6 | DMA_HISR_FEIF6 ) ) != 0 )
  {
    /* Clear interrupt */
    DMA1->HIFCR |= ( DMA_HISR_TEIF6 | DMA_HISR_DMEIF6 | DMA_HISR_FEIF6 );
    
    if ( tx_complete == false )
    {
      uart_interfaces[ STM32_UART_2 ].tx_dma_result = kGeneralErr;
    }
  }
  
#ifndef NO_MICO_RTOS
  /* Set semaphore regardless of result to prevent waiting thread from locking up */
  mico_rtos_set_semaphore( &uart_interfaces[ STM32_UART_2 ].tx_complete);
#else
  uart_interfaces[ STM32_UART_2 ].tx_complete = true;
#endif
}

//usart6_tx_dma_irq
void DMA2_Stream6_IRQHandler( void )
{
  bool tx_complete = false;
  
  if ( ( DMA2->HISR & DMA_HISR_TCIF6 ) != 0 )
  {
    DMA2->HIFCR |= DMA_HISR_TCIF6;
    uart_interfaces[ STM32_UART_6 ].tx_dma_result = kNoErr;
    tx_complete = true;
  }
  
  /* TX DMA error */
  if ( ( DMA2->HISR & ( DMA_HISR_TEIF6 | DMA_HISR_DMEIF6 | DMA_HISR_FEIF6 ) ) != 0 )
  {
    /* Clear interrupt */
    DMA2->HIFCR |= ( DMA_HISR_TEIF6 | DMA_HISR_DMEIF6 | DMA_HISR_FEIF6 );
    
    if ( tx_complete == false )
    {
      uart_interfaces[ STM32_UART_6 ].tx_dma_result = kGeneralErr;
    }
  }
  
#ifndef NO_MICO_RTOS
  /* Set semaphore regardless of result to prevent waiting thread from locking up */
  mico_rtos_set_semaphore( &uart_interfaces[ STM32_UART_6 ].tx_complete);
#else
  uart_interfaces[ STM32_UART_6 ].tx_complete = true;
#endif
  
}

//usart1_rx_dma_irq
void DMA2_Stream2_IRQHandler( void )
{
  if ( ( DMA2->LISR & DMA_LISR_TCIF2 ) != 0 )
  {
    DMA2->LIFCR |= DMA_LISR_TCIF2;
    uart_interfaces[ STM32_UART_1 ].rx_dma_result = kNoErr;
  }
  
  /* RX DMA error */
  if ( ( DMA2->LISR & ( DMA_LISR_TEIF2 | DMA_LISR_DMEIF2 | DMA_LISR_FEIF2 ) ) != 0 )
  {
    /* Clear interrupt */
    DMA2->LIFCR |= ( DMA_LISR_TEIF2 | DMA_LISR_DMEIF2 | DMA_LISR_FEIF2 );
    uart_interfaces[ STM32_UART_1 ].rx_dma_result = kGeneralErr;
  }
#ifndef NO_MICO_RTOS
  mico_rtos_set_semaphore( &uart_interfaces[ STM32_UART_1 ].rx_complete );
#else
  uart_interfaces[ STM32_UART_1 ].rx_complete = true;
#endif
}

//usart2_rx_dma_irq
void DMA1_Stream5_IRQHandler( void )
{
  if ( ( DMA1->HISR & DMA_HISR_TCIF5 ) != 0 )
  {
    DMA1->LIFCR |= DMA_HISR_TCIF5;
    uart_interfaces[ STM32_UART_2 ].rx_dma_result = kNoErr;
  }
  
  /* RX DMA error */
  if ( ( DMA1->HISR & ( DMA_HISR_TEIF5 | DMA_HISR_DMEIF5 | DMA_HISR_FEIF5 ) ) != 0 )
  {
    /* Clear interrupt */
    DMA1->LIFCR |= ( DMA_HISR_TEIF5 | DMA_HISR_DMEIF5 | DMA_HISR_FEIF5 );
    uart_interfaces[ STM32_UART_2 ].rx_dma_result = kGeneralErr;
  }
#ifndef NO_MICO_RTOS
  mico_rtos_set_semaphore( &uart_interfaces[ STM32_UART_2 ].rx_complete );
#else
  uart_interfaces[ STM32_UART_2 ].rx_complete = true;
#endif
}

//usart6_rx_dma_irq
void DMA2_Stream1_IRQHandler( void )
{
  if ( ( DMA2->LISR & DMA_LISR_TCIF1 ) != 0 )
  {
    DMA2->LIFCR |= DMA_LISR_TCIF1;
    uart_interfaces[ STM32_UART_6 ].rx_dma_result = kNoErr;
    
  }
  
  /* TX DMA error */
  if ( ( DMA2->LISR & ( DMA_LISR_TEIF1 | DMA_LISR_DMEIF1 | DMA_LISR_FEIF1 ) ) != 0 )
  {
    /* Clear interrupt */
    DMA2->LIFCR |= ( DMA_LISR_TEIF1 | DMA_LISR_DMEIF1 | DMA_LISR_FEIF1 );
    uart_interfaces[ STM32_UART_6 ].rx_dma_result = kGeneralErr;
  }
  
#ifndef NO_MICO_RTOS
  mico_rtos_set_semaphore( &uart_interfaces[ STM32_UART_6 ].rx_complete );
#else
  uart_interfaces[ STM32_UART_6 ].rx_complete = true;
#endif
}




