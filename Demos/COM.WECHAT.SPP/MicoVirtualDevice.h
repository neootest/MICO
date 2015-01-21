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

/* init */
OSStatus MVDInit(mico_Context_t* const context);
void MVDRestoreDefault(mico_Context_t* const context);

/* get MVD state */

// dev activate state
bool MVDIsActivated(mico_Context_t* const context);
// cloud connect state
bool MVDCloudIsConnect(mico_Context_t* const context);

/* MVD message send interface */

// MVD => MCU
OSStatus MVDSendMsg2Device(mico_Context_t* const context, 
                           unsigned char *inBuf, unsigned int inBufLen);
// MVD => Cloud
OSStatus MVDSendMsg2Cloud(mico_Context_t* const context, const char* topic,
                       unsigned char *inBuf, unsigned int inBufLen);


/* device control */

//OTA
OSStatus MVDFirmwareUpdate(mico_Context_t* const context,
                           MVDOTARequestData_t OTAData);
//activate
OSStatus MVDActivate(mico_Context_t* const context, 
                     MVDActivateRequestData_t activateData);
//authorize
OSStatus MVDAuthorize(mico_Context_t* const context,
                      MVDAuthorizeRequestData_t authorizeData);
//reset device info on cloud
OSStatus MVDResetCloudDevInfo(mico_Context_t* const context,
                              MVDResetRequestData_t devResetData);


/*******************************************************************************
* INTERNAL FUNCTIONS
*******************************************************************************/

/* message exchage protocol */

// MCU => Cloud, called by MVDDeviceInterface (when msg recv)
OSStatus MVDDeviceMsgProcess(mico_Context_t* const context, 
                             unsigned char *inBuf, unsigned int inBufLen);

// Cloud => MCU, called by MVDCloudInterface (when msg recv)
OSStatus MVDCloudMsgProcess(mico_Context_t* const context, 
                            const char* topic, const unsigned int topicLen,
                            unsigned char *inBuf, unsigned int inBufLen);

#endif
