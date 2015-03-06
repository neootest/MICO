/**
******************************************************************************
* @file    MVDDeviceInterfaces.c
* @author  Eshen Wang
* @version V0.2.0
* @date    21-Nov-2014
* @brief   This file contains the implementations of lower device interfaces 
*          for MICO virtual device. 
  operation
******************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, MXCHIP Inc. SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* <h2><center>&copy; COPYRIGHT 2014 MXCHIP Inc.</center></h2>
******************************************************************************
*/ 

#include <stdio.h>

#include "MICODefine.h"
#include "MicoPlatform.h"

#include "MVDDeviceInterfaces.h"
#include "MicoVirtualDevice.h"


#define dev_if_log(M, ...) custom_log("MVD_DEV_IF", M, ##__VA_ARGS__)
#define dev_if_log_trace() custom_log_trace("MVD_DEV_IF")

volatile ring_buffer_t  rx_buffer;
volatile uint8_t        rx_data[UART_BUFFER_LENGTH];

//extern void uartRecv_thread(void *inContext);
static size_t _uart_get_one_packet(uint8_t* buf, int maxlen);

void uartRecv_thread(void *inContext)
{
  dev_if_log_trace();
  mico_Context_t *Context = inContext;
  int recvlen;
  uint8_t *inDataBuffer;
  
  inDataBuffer = malloc(UART_ONE_PACKAGE_LENGTH);
  require(inDataBuffer, exit);
  
  while(1) {
    recvlen = _uart_get_one_packet(inDataBuffer, UART_ONE_PACKAGE_LENGTH);
    if (recvlen <= 0)
      continue; 
    dev_if_log("MCU => MVD: [%d]=%.*s", recvlen, recvlen, inDataBuffer);
    MVDDeviceMsgProcess(Context, inDataBuffer, recvlen);
  }
  
exit:
  if(inDataBuffer) free(inDataBuffer);
}

/* Packet format: BB 00 CMD(2B) Status(2B) datalen(2B) data(x) checksum(2B)
* copy to buf, return len = datalen+10
*/
size_t _uart_get_one_packet(uint8_t* inBuf, int inBufLen)
{
  dev_if_log_trace();

  int datalen;
  
  while(1) {
    if( MicoUartRecv( UART_FOR_MCU, inBuf, inBufLen, UART_RECV_TIMEOUT) == kNoErr){
      return inBufLen;
    }
   else{
     datalen = MicoUartGetLengthInBuffer( UART_FOR_MCU );
     if(datalen){
       MicoUartRecv(UART_FOR_MCU, inBuf, datalen, UART_RECV_TIMEOUT);
       return datalen;
     }
   }  
  } 
}


/*******************************************************************************
* INTERFACES
******************************************************************************/

OSStatus MVDDevInterfaceInit(mico_Context_t* const inContext)
{
  OSStatus err = kUnknownErr;
  mico_uart_config_t uart_config;
  
  //USART init
  uart_config.baud_rate    = inContext->flashContentInRam.appConfig.virtualDevConfig.USART_BaudRate;
  uart_config.data_width   = DATA_WIDTH_8BIT;
  uart_config.parity       = NO_PARITY;
  uart_config.stop_bits    = STOP_BITS_1;
  uart_config.flow_control = FLOW_CONTROL_DISABLED;
  if(inContext->flashContentInRam.micoSystemConfig.mcuPowerSaveEnable == true)
    uart_config.flags = UART_WAKEUP_ENABLE;
  else
    uart_config.flags = UART_WAKEUP_DISABLE;
  ring_buffer_init  ( (ring_buffer_t *)&rx_buffer, (uint8_t *)rx_data, UART_BUFFER_LENGTH );
  
  MicoUartInitialize( UART_FOR_MCU, &uart_config, (ring_buffer_t *)&rx_buffer );
  
  //USART receive thread
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART Recv", 
                                uartRecv_thread, STACK_SIZE_USART_RECV_THREAD, 
                                (void*)inContext );
  require_noerr_action( err, exit, dev_if_log("ERROR: Unable to start the USART recv thread.") );
  return kNoErr;
  
exit:
  return err;
}

OSStatus MVDDevInterfaceSend(unsigned char *inBuf, unsigned int inBufLen)
{
  OSStatus err = kUnknownErr;

  dev_if_log("MVD => MCU:[%d]=%.*s", inBufLen, inBufLen, inBuf);
  err = MicoUartSend(UART_FOR_MCU, inBuf, inBufLen);
  require_noerr_action( err, exit, dev_if_log("ERROR: send to USART error! err=%d", err) );
  return kNoErr;
  
exit:
  return err;
}
