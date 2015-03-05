/**
******************************************************************************
* @file    MicoVirtualDevice.h 
* @author  Eshen Wang
* @version V0.2.0
* @date    21-Nov-2014
* @brief   This header contains the interfaces 
*          of MICO virtual device. 
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

#ifndef __MICO_MVD_H_
#define __MICO_MVD_H_

#include "MICODefine.h"
#include "MicoVirtualDeviceDef.h"


/*******************************************************************************
 * USER INTERFACES
 ******************************************************************************/

/***** init *****/
   
// init USART && Cloud interface
OSStatus MVDInit(mico_Context_t* const context);
// restore default config for MVD
void MVDRestoreDefault(mico_Context_t* const context);


/***** get MVD state *****/

// device activate state
bool MVDIsActivated(mico_Context_t* const context);
// cloud connect state
bool MVDCloudIsConnect(mico_Context_t* const context);


/****** send message ******/

// Module => MCU
OSStatus MVDSendMsg2Device(mico_Context_t* const context, 
                           unsigned char *inBuf, unsigned int inBufLen);
// Module => Cloud
OSStatus MVDSendMsg2Cloud(mico_Context_t* const context, const char* topic,
                       unsigned char *inBuf, unsigned int inBufLen);


/*******************************************************************************
* INTERNAL FUNCTIONS
*******************************************************************************/

/* message transmit protocol */

// MCU => Cloud, called by uartRecv_thread (when recv msg from MCU)
OSStatus MVDDeviceMsgProcess(mico_Context_t* const context, 
                             unsigned char *inBuf, unsigned int inBufLen);

// Cloud => MCU, called by cloudMsgArrivedHandler (when recv msg from cloud)
OSStatus MVDCloudMsgProcess(mico_Context_t* const context, 
                            const char* topic, const unsigned int topicLen,
                            unsigned char *inBuf, unsigned int inBufLen);

#endif
