/**
******************************************************************************
* @file    MVDCloudInterfaces.h 
* @author  Eshen Wang
* @version V0.2.0
* @date    21-Nov-2014
* @brief   This header contains the cloud service interfaces 
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


#ifndef __MICO_MVDCLOUDINTERFACES_H_
#define __MICO_MVDCLOUDINTERFACES_H_


#include "MICODefine.h"
#include "EasyCloudServiceDef.h"

/*******************************************************************************
 * DEFINES
 ******************************************************************************/


/*******************************************************************************
 * INTERFACES
 ******************************************************************************/

//common interfaces
OSStatus MVDCloudInterfaceInit(mico_Context_t* const inContext);
OSStatus MVDCloudInterfaceStart(mico_Context_t* const inContext);
easycloud_service_state_t MVDCloudInterfaceGetState(void);
OSStatus MVDCloudInterfaceSend(unsigned char *inBuf, unsigned int inBufLen);
OSStatus MVDCloudInterfaceSendto(const char* topic, 
                                 unsigned char *inBuf, unsigned int inBufLen);
// send to sub-level, topic "device_id/out/<level>"
OSStatus MVDCloudInterfaceSendtoChannel(const char* channel, 
                                      unsigned char *inBuf, unsigned int inBufLen);

// cloud specifical interfaces
OSStatus MVDCloudInterfaceDevActivate(mico_Context_t* const inContext,
                                      MVDActivateRequestData_t devActivateReqData);
OSStatus MVDCloudInterfaceDevAuthorize(mico_Context_t* const inContext,
                                       MVDAuthorizeRequestData_t devAuthorizeReqData);

OSStatus MVDCloudInterfaceDevFirmwareUpdate(mico_Context_t* const inContext,
                                            MVDOTARequestData_t devOTARequestData);
OSStatus MVDCloudInterfaceResetCloudDevInfo(mico_Context_t* const inContext,
                                            MVDResetRequestData_t devResetRequestData);

OSStatus MVDCloudInterfaceStop(mico_Context_t* const inContext);
OSStatus MVDCloudInterfaceDeinit(mico_Context_t* const inContext);
OSStatus MVDCloudInterfacePrintVersion(void);

#endif
