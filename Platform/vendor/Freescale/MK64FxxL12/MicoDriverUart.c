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
#include <string.h>
#include "fsl_uart_driver.h"
#include "fsl_uart_common.h"
#include "fsl_clock_manager.h"
#include "fsl_interrupt_manager.h"

#include "board.h"
#include "PlatformLogging.h"
/******************************************************
*                    Constants
******************************************************/
/* // a plan 
const platform_uart_mapping_t uart_mapping[] =
{
  [MICO_UART_1] =
  {
    .usart                        = USART2,
    .gpio_af                      = GPIO_AF_USART2,
  }
};*/ 
/******************************************************
*                   Enumerations
******************************************************/

/******************************************************
*                 Type Definitions
******************************************************/
#define UART_APP_INDEX 1 //test Jer
//#define USE_DMA_UART
/* Ring buffer size */
#define UART_RB_SIZE 128

#define UART_IRQ_APP 


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



#ifdef UART_IRQ_APP
static uart_user_config_t uartConfig;
static uart_state_t uartState_g;
#else
static edma_state_t state_app; 
static edma_user_config_t userConfig_app; 
static uart_edma_state_t uartStateEdma_app;    
static uart_edma_user_config_t uartConfig_app;
#endif

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
    
    uint32_t baseAddr = g_uartBaseAddr[UART_APP_INDEX];
    uint32_t uartSourceClock;
    uart_state_t* uartStatePtr = &uartState_g;
    /* Create Semaphore . */
#ifndef NO_MICO_RTOS
  mico_rtos_init_semaphore(&uart_interfaces[1].tx_complete, 1);
  mico_rtos_init_semaphore(&uart_interfaces[1].rx_complete, 1);
#else
  uart_interfaces[1].tx_complete = false;
  uart_interfaces[1].rx_complete = false;
#endif

    /* Clear the state structure for this UART_APP_INDEX. */
    memset(uartStatePtr, 0, sizeof(uart_state_t));

    /* Save runtime structure pointer.*/
    g_uartStatePtr[UART_APP_INDEX] = uartStatePtr;

    /* Un-gate UART module clock */
    CLOCK_SYS_EnableUartClock(UART_APP_INDEX);

    /* Initialize UART to a known state. */
    UART_HAL_Init(baseAddr);

    /* UART clock source is either system clock or bus clock depending on the UART_APP_INDEX */
    uartSourceClock = CLOCK_SYS_GetUartFreq(UART_APP_INDEX);

    uartConfig.baudRate = 115200; //test Jer
    uartConfig.bitCountPerChar = kUart8BitsPerChar;
    uartConfig.parityMode = kUartParityDisabled;
    uartConfig.stopBitCount = kUartOneStopBit;
    
    /* Initialize UART baud rate, bit count, parity and stop bit. */
    UART_HAL_SetBaudRate(baseAddr, uartSourceClock, 115200);
    UART_HAL_SetBitCountPerChar(baseAddr, kUart8BitsPerChar);
    UART_HAL_SetParityMode(baseAddr, kUartParityDisabled);
#if FSL_FEATURE_UART_HAS_STOP_BIT_CONFIG_SUPPORT
    UART_HAL_SetStopBitCount(baseAddr, kUartOneStopBit);
#endif

#ifndef NO_MICO_RTOS
  if(config->flags & UART_WAKEUP_ENABLE){
    current_uart = uart;
    mico_rtos_init_semaphore( &uart_interfaces[1].sem_wakeup, 1 );
    mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART_WAKEUP", thread_wakeup, 0x100, &current_uart);
  }
#endif
#if FSL_FEATURE_UART_HAS_FIFO
    uint8_t fifoSize;
    /* Obtain raw TX FIFO size bit setting */
    fifoSize = UART_HAL_GetTxFifoSize(baseAddr);
    /* Now calculate the number of data words per given FIFO size */
    uartStatePtr->txFifoEntryCount = (fifoSize == 0 ? 1 : 0x1 << (fifoSize + 1));

    /* Configure the TX FIFO watermark to be 1/2 of the total entry or 0 if entry count = 1
     * A watermark setting of 0 for TX FIFO entry count of 1 means that TDRE will only interrupt
     * when the TX buffer (the one entry in the TX FIFO) is empty. Otherwise, if we set the
     * watermark to 1, the TDRE will always be set regardless if the TX buffer was empty or not
     * as the spec says TDRE will set when the FIFO is at or below the configured watermark. */
    if (uartStatePtr->txFifoEntryCount > 1)
    {
        UART_HAL_SetTxFifoWatermark(baseAddr, (uartStatePtr->txFifoEntryCount >> 1U));
    }
    else
    {
        UART_HAL_SetTxFifoWatermark(baseAddr, 0);
    }

    /* Configure the RX FIFO watermark to be 1. 
     * Note about RX FIFO support: There is only one RX data full interrupt that is
     * associated with the RX FIFO Watermark.  The watermark cannot be dynamically changed.
     * This means if the rxSize is less than the programmed watermark the interrupt will 
     * never occur. If we try to change the watermark, this will involve shutting down
     * the receiver first - which is not a desirable operation when the UART is actively
     * receiving data. Hence, the best solution is to set the RX FIFO watermark to 1. */
    UART_HAL_SetRxFifoWatermark(baseAddr, 1);

    /* Enable and flush the FIFO prior to enabling the TX/RX */
    UART_HAL_SetTxFifoCmd(baseAddr, true);
    UART_HAL_SetRxFifoCmd(baseAddr, true);
    UART_HAL_FlushTxFifo(baseAddr);
    UART_HAL_FlushRxFifo(baseAddr);
#else
    /* For modules that do not support a FIFO, they have a data buffer that essentially
     * acts likes a one-entry FIFO, thus to make the code cleaner, we'll
     * equate txFifoEntryCount to 1.  Also note that TDRE flag will set only when the tx
     * buffer is empty. */
    uartStatePtr->txFifoEntryCount = 1;
#endif

    /* Enable UART interrupt on NVIC level. */
    INT_SYS_EnableIRQ(g_uartRxTxIrqId[UART_APP_INDEX]);

    /* Finally, enable the UART transmitter and receiver*/
    UART_HAL_EnableTransmitter(baseAddr);
    UART_HAL_EnableReceiver(baseAddr);

#ifdef USE_DMA_UART

#endif

  if (optional_rx_buffer != NULL)
  {
     //  Note that the ring_buffer should've been initialised first
    uart_interfaces[1].rx_buffer = optional_rx_buffer;
    uart_interfaces[1].rx_size   = 0;
    platform_uart_receive_bytes( uart, optional_rx_buffer->buffer, optional_rx_buffer->size, 0 );
  }  
  MicoMcuPowerSaveConfig(true);  
  return kNoErr;
}

OSStatus MicoUartFinalize( mico_uart_t uart )
{
  
    uint32_t baseAddr = g_uartBaseAddr[UART_APP_INDEX];

    /* In case there is still data in the TX FIFO or shift register that is being transmitted
     * wait till transmit is complete. */
#if FSL_FEATURE_UART_HAS_FIFO
    /* Wait until there all of the data has been drained from the TX FIFO */
    while(UART_HAL_GetTxDatawordCountInFifo(baseAddr) != 0) { }
#endif
    /* Wait until the data is completely shifted out of shift register */
    while(!(UART_HAL_IsTxComplete(baseAddr))) { }

    /* Disable the interrupt */
    INT_SYS_DisableIRQ(g_uartRxTxIrqId[UART_APP_INDEX]);

    /* Disable TX and RX */
    UART_HAL_DisableTransmitter(baseAddr);
    UART_HAL_DisableReceiver(baseAddr);

#if FSL_FEATURE_UART_HAS_FIFO
    /* Disable the FIFOs; should be done after disabling the TX/RX */
    UART_HAL_SetTxFifoCmd(baseAddr, false);
    UART_HAL_SetRxFifoCmd(baseAddr, false);
    UART_HAL_FlushTxFifo(baseAddr);
    UART_HAL_FlushRxFifo(baseAddr);
#endif

    /* Cleared state pointer. */
    g_uartStatePtr[UART_APP_INDEX] = NULL;

    /* Gate UART module clock */
    CLOCK_SYS_DisableUartClock(UART_APP_INDEX);
#ifndef NO_MICO_RTOS
  mico_rtos_deinit_semaphore(&uart_interfaces[1].rx_complete);
  mico_rtos_deinit_semaphore(&uart_interfaces[1].tx_complete);
#endif  
  
  return kNoErr;
}

void UART_DRV_CompleteSendData(uint32_t instance)
{
   // assert(instance < HW_UART_UART_APP_INDEX_COUNT);

    uint32_t baseAddr = g_uartBaseAddr[instance];
    uart_state_t * uartState = (uart_state_t *)g_uartStatePtr[instance];
platform_log("CompleteSend");
    /* Disable the transmitter data register empty interrupt */
    UART_HAL_SetTxDataRegEmptyIntCmd(baseAddr, false);

    /* Signal the synchronous completion object. */
    if (uartState->isTxBlocking)
    {
        //OSA_SemaPost(&uartState->txIrqSync);
        mico_rtos_set_semaphore(&uart_interfaces[1].tx_complete);
    }

    /* Update the information of the module driver state */
    uartState->isTxBusy = false; 
}

OSStatus MicoUartSend( mico_uart_t uart, const void* data, uint32_t size )
{
#ifndef USE_DMA_UART
  uart_interfaces[1].tx_dma_result = kGeneralErr;
  MicoMcuPowerSaveConfig(false);  
    uint32_t baseAddr = g_uartBaseAddr[UART_APP_INDEX];
    uart_state_t * uartState = (uart_state_t *)g_uartStatePtr[UART_APP_INDEX];
platform_log("UartSend");
    /* Indicates current transaction is non-blocking.*/
    uartState->isTxBlocking = false;

    /* Check that we're not busy already transmitting data from a previous function call. */
    if (uartState->isTxBusy)
    {
platform_log("UartSend1");
        return kGeneralErr; //kStatus_UART_TxBusy;
    }

    /* Initialize the module driver state structure. */
    uartState->txBuff = data ;//txBuff;
    uartState->txSize = size;
    uartState->isTxBusy = true;

    /* Fill the TX FIFO or TX data buffer. In the event that there still might be data in the
     * TX FIFO, first ascertain the nubmer of empty spaces and then fill those up. */
    uint8_t emptyEntryCountInFifo;
#if FSL_FEATURE_UART_HAS_FIFO
    emptyEntryCountInFifo = uartState->txFifoEntryCount -
                            UART_HAL_GetTxDatawordCountInFifo(baseAddr);
#else
    /* For modules that don't have a FIFO, there is no FIFO data count register */
    emptyEntryCountInFifo = uartState->txFifoEntryCount;
    /* Make sure the transmit data register is empty and ready for data */
    while(!UART_HAL_IsTxDataRegEmpty(baseAddr)) { }
#endif

    /* Fill up FIFO, if only a 1-entry FIFO, then just fill the data buffer */
    while(emptyEntryCountInFifo--)
    {
        /* put data into FIFO */
        UART_HAL_Putchar(baseAddr, *(uartState->txBuff)); 
platform_log("UartSend2");
        ++uartState->txBuff;
        --uartState->txSize;
        /* If there are no more bytes in the buffer to send, then complete transmit. No need
         * to spend time enabling the interrupt and going to the ISR.  */
        if (uartState->txSize == 0)
        {
platform_log("UartSend3");
            UART_DRV_CompleteSendData(UART_APP_INDEX);
            return kNoErr;
        }
    }
platform_log("UartSend4");
    /* Enable the transmitter data register empty interrupt. The TDRE flag will set whenever
     * the TX buffer is emptied into the TX shift register (for non-FIFO IPs) or when the
     * data in the TX FIFO is at or below the programmed watermark (for FIFO-supported IPs). */
    UART_HAL_SetTxDataRegEmptyIntCmd(baseAddr, true);
  return kNoErr;

#else 
//  /* Reset DMA transmission result. The result is assigned in interrupt handler */
  uart_interfaces[1].tx_dma_result = kGeneralErr;
  MicoMcuPowerSaveConfig(false);  
TODO!

#ifndef NO_MICO_RTOS
  mico_rtos_get_semaphore( &uart_interfaces[ uart ].tx_complete, MICO_NEVER_TIMEOUT );
#else 
  while(uart_interfaces[ uart ].tx_complete == false);
  uart_interfaces[ uart ].tx_complete = false;
#endif
  MicoMcuPowerSaveConfig(true);
  return uart_interfaces[1].tx_dma_result; 
#endif

}

void UART_DRV_CompleteReceiveData(uint32_t instance)
{
    //assert(UART_APP_INDEX < HW_UART_UART_APP_INDEX_COUNT);
    uart_state_t * uartState = (uart_state_t *)g_uartStatePtr[instance];
    uint32_t baseAddr = g_uartBaseAddr[instance];
platform_log("CompleteRecv");
    /* Disable receive data full interrupt */
    UART_HAL_SetRxDataRegFullIntCmd(baseAddr, false);

    /* Signal the synchronous completion object. */
    if (uartState->isRxBlocking)
    {
        //OSA_SemaPost(&uartState->rxIrqSync);
        mico_rtos_set_semaphore(&uart_interfaces[1].rx_complete);
    }

    /* Update the information of the module driver state */
    uartState->isRxBusy = false;
}

OSStatus MicoUartRecv( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{
#if 0
  if (uart_interfaces[1].rx_buffer != NULL)
  {
    while (size != 0)
    {
      uint32_t transfer_size = MIN(uart_interfaces[1].rx_buffer->size / 2, size);
     platform_log("UartRecvRingB 1"); 
      /* Check if ring buffer already contains the required amount of data. */
      if ( transfer_size > ring_buffer_used_space( uart_interfaces[1].rx_buffer ) )
      {
        /* Set rx_size and wait in rx_complete semaphore until data reaches rx_size or timeout occurs */
        uart_interfaces[1].rx_size = transfer_size;
#ifndef NO_MICO_RTOS
        if ( mico_rtos_get_semaphore( &uart_interfaces[1].rx_complete, timeout) != kNoErr )
        {
          uart_interfaces[1].rx_size = 0;
          return kTimeoutErr;
        }
#else
        uart_interfaces[1].rx_complete = false;
        int delay_start = mico_get_time_no_os();
        while(uart_interfaces[1].rx_complete == false){
          if(mico_get_time_no_os() >= delay_start + timeout && timeout != MICO_NEVER_TIMEOUT){
            uart_interfaces[1].rx_size = 0;
            return kTimeoutErr;
          }
        }
#endif
        /* Reset rx_size to prevent semaphore being set while nothing waits for the data */
        uart_interfaces[1].rx_size = 0;
      }
     
      size -= transfer_size;
      
      // Grab data from the buffer
      do
      {
        uint8_t* available_data;
        uint32_t bytes_available;
        ring_buffer_get_data( uart_interfaces[1].rx_buffer, &available_data, &bytes_available );
        bytes_available = MIN( bytes_available, transfer_size );
        memcpy( data, available_data, bytes_available );
        transfer_size -= bytes_available;
        data = ( (uint8_t*) data + bytes_available );
        ring_buffer_consume( uart_interfaces[1].rx_buffer, bytes_available );
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
#else
    return platform_uart_receive_bytes( uart, data, size, timeout );
#endif
}


static OSStatus platform_uart_receive_bytes( mico_uart_t uart, void* data, uint32_t size, uint32_t timeout )
{
  /* Reset DMA transmission result. The result is assigned in interrupt handler */
   uart_interfaces[1].rx_dma_result = kGeneralErr;
    uart_state_t * uartState = (uart_state_t *)g_uartStatePtr[UART_APP_INDEX];
    uint32_t baseAddr = g_uartBaseAddr[UART_APP_INDEX];
    uart_status_t error = kStatus_UART_Success;
    osa_status_t syncStatus;
    /* Indicates current transaction is blocking.*/
    uartState->isRxBlocking = true;

    /* Initialize the module driver state struct to indicate transfer in progress
     * and with the buffer and byte count data */
    uartState->rxBuff = data;
    uartState->rxSize = size;
    uartState->isRxBusy = true;

    /* enable the receive data full interrupt */
    UART_HAL_SetRxDataRegFullIntCmd(baseAddr, true);
  //  if (UART_DRV_StartReceiveData(UART_APP_INDEX, rxBuff, rxSize) == kStatus_UART_RxBusy)
  //  {
  //    return kStatus_UART_RxBusy;
  //  }

  if ( timeout > 0 )
   {
#ifndef NO_MICO_RTOS
   //return mico_rtos_get_semaphore( &uart_interfaces[1].rx_complete, timeout );
    if(kNoErr != mico_rtos_get_semaphore( &uart_interfaces[1].rx_complete, timeout )){
        /* Abort the transfer so it doesn't continue unexpectedly.*/
        UART_DRV_AbortReceivingData(UART_APP_INDEX);
        /* Check if a transfer is running. */
        if (!uartState->isRxBusy)
        {
            return kGeneralErr;//kNoErr; //kStatus_UART_NoReceiveInProgress;
        }
        /* Stop the running transfer. */
       // UART_DRV_CompleteReceiveData(UART_APP_INDEX);
        return kGeneralErr;              
    }
#else
    uart_interfaces[1].rx_complete = false;
    int delay_start = mico_get_time_no_os();
    while(uart_interfaces[1].rx_complete == false){
      if(mico_get_time_no_os() >= delay_start + timeout && timeout != MICO_NEVER_TIMEOUT){
        break;
      }
    }    
#endif
    return uart_interfaces[1].rx_dma_result;
  }  
  platform_log("uart receive 4");
  return kGeneralErr;              // kNoErr;
}


uint32_t MicoUartGetLengthInBuffer( mico_uart_t uart )
{
  return ring_buffer_used_space( uart_interfaces[1].rx_buffer );
}

#ifndef NO_MICO_RTOS
static void thread_wakeup(void *arg)
{
  mico_uart_t uart = *(mico_uart_t *)arg;
  
  while(1){
     if(mico_rtos_get_semaphore(&uart_interfaces[ uart ].sem_wakeup, 1000) != kNoErr){
//      gpio_irq_enable(uart_mapping[1].pin_rx->bank, uart_mapping[1].pin_rx->number, IRQ_TRIGGER_FALLING_EDGE, RX_PIN_WAKEUP_handler, &uart);
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
  
//  RCC_AHB1PeriphClockCmd(uart_mapping[ uart ].pin_rx->peripheral_clock, ENABLE);
//  uart_mapping[ uart ].usart_peripheral_clock_func ( uart_mapping[1].usart_peripheral_clock,  ENABLE );
//  uart_mapping[1].rx_dma_peripheral_clock_func  ( uart_mapping[1].rx_dma_peripheral_clock, ENABLE );
//  
//  gpio_irq_disable(uart_mapping[1].pin_rx->bank, uart_mapping[1].pin_rx->number); 
  
  MicoMcuPowerSaveConfig(false);
  mico_rtos_set_semaphore(&uart_interfaces[1].sem_wakeup);
}
#endif

/*FUNCTION**********************************************************************
 *
 * Function Name : UART_DRV_IRQHandler
 * Description   : Interrupt handler for UART.
 * This handler uses the buffers stored in the uart_state_t structs to transfer data.
 * This is not a public API as it is called whenever an interrupt occurs.
 *
 *END**************************************************************************/
#if 0
void UART_DRV_IRQHandler(uint32_t instance)
{
    uart_state_t * uartState = (uart_state_t *)g_uartStatePtr[instance];
    uint32_t baseAddr = g_uartBaseAddr[instance];
    bool rxCallbackEnd = false;

    /* Exit the ISR if no transfer is happening for this UART_APP_INDEX. */
    if ((!uartState->isTxBusy) && (!uartState->isRxBusy))
    {
        return;
    }

    /* Check to see if the interrupt is due to receive data full
     * first see if the interrupt is enabled. */
    if(UART_HAL_GetRxDataRegFullIntCmd(baseAddr))
    {
        if(UART_HAL_IsRxDataRegFull(baseAddr))
        {
#if FSL_FEATURE_UART_HAS_FIFO
            /* Read from RX FIFO while the RX count indicates there's data in the FIFO.
             * Even though the watermark is set to 1, it might be possible to have more than one
             * byte in the FIFO, so lets make sure to drain it. */
            while(UART_HAL_GetRxDatawordCountInFifo(baseAddr))
            {
#endif
                /* Get data and put in receive buffer */
                UART_HAL_Getchar(baseAddr, uartState->rxBuff);

                /* Invoke callback if have one. */
                if (uartState->rxCallback != NULL)
                {
                    /* The callback will end the receiving early if not return
                     * kStatus_UART_Success. */
                    if (uartState->rxCallback(uartState->rxBuff, uartState->rxCallbackParam) !=
                            kStatus_UART_Success)
                    {
                        rxCallbackEnd = true;
                    }
                }

                ++uartState->rxBuff;  /* Increment the rxBuff pointer */
                --uartState->rxSize;  /* Decrement the byte count  */

                /* Check to see if this was the last byte received */
                if ((uartState->rxSize == 0) || rxCallbackEnd)
                {
                    /* Complete the transfer. This disables the interrupts, so we don't wind up in*/
                    /* the ISR again. */
                    UART_DRV_CompleteReceiveData(instance);
                    #if FSL_FEATURE_UART_HAS_FIFO
                    break;
                    #endif
                }
            
#if FSL_FEATURE_UART_HAS_FIFO
            }
#endif
        }
    }

    /* Check to see if the interrupt is due to transmit data register empty
     * first see if the interrupt is enabled. */
    if (UART_HAL_GetTxDataRegEmptyIntCmd(baseAddr))
    {
        if(UART_HAL_IsTxDataRegEmpty(baseAddr))
        {
            /* Check to see if there are any more bytes to send */
            if (uartState->txSize)
            {
                uint8_t emptyEntryCountInFifo;
#if FSL_FEATURE_UART_HAS_FIFO
                emptyEntryCountInFifo = uartState->txFifoEntryCount -
                                        UART_HAL_GetTxDatawordCountInFifo(baseAddr);
#else
                /* For modules that don't have a FIFO, there is no FIFO data count register */
                emptyEntryCountInFifo = uartState->txFifoEntryCount;
#endif

                /* Fill up FIFO, if only a 1-entry FIFO, then just fill the data buffer */
                while(emptyEntryCountInFifo--)
                {
                    /* Transmit data and update tx size/buff. */
                    UART_HAL_Putchar(baseAddr, *(uartState->txBuff));
                    ++uartState->txBuff; 
                    --uartState->txSize;

                    /* If there are no more bytes in the buffer to send, complete the transmit
                     * process and break out of the while loop. We should not re-enter the ISR again
                     * as all of the data has been put into the FIFO (or the TX data buffer). */
                    if (!uartState->txSize)
                    {
                        UART_DRV_CompleteSendData(instance);
                        break;
                    }
                }
            }
        }
    }
}
#endif

// end file






