/**
  ******************************************************************************
  * @file    MICOAppEntrance.c 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   Mico application entrance, addd user application functons and threads.
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

#include "MICO.h"
#include "MICODefine.h"
#include "MICOAppDefine.h"

#include "StringUtils.h"
#include "SppProtocol.h"

#include "Platform.h"
#include "PlatformUART.h"

#define app_log(M, ...) custom_log("APP", M, ##__VA_ARGS__)
#define app_log_trace() custom_log_trace("APP")

/* MICO system callback: Restore default configuration provided by application */
void appRestoreDefault_callback(mico_Context_t *inContext)
{
  inContext->flashContentInRam.appConfig.localServerPort = LOCAL_PORT;
  inContext->flashContentInRam.appConfig.localServerEnable = true;
  inContext->flashContentInRam.appConfig.USART_BaudRate = 115200;
  inContext->flashContentInRam.appConfig.remoteServerEnable = true;
  sprintf(inContext->flashContentInRam.appConfig.remoteServerDomain, DEAFULT_REMOTE_SERVER);
  inContext->flashContentInRam.appConfig.remoteServerPort = DEFAULT_REMOTE_SERVER_PORT;
  
  
}

OSStatus MICOStartApplication( mico_Context_t * const inContext )
{
  app_log_trace();

  OSStatus err = kNoErr;
  require_action(inContext, exit, err = kParamErr);

  sppProtocolInit(inContext);
  PlatformUartInitialize(inContext);

  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART Recv", uartRecv_thread, 0x500, (void*)inContext );
  require_noerr_action( err, exit, app_log("ERROR: Unable to start the uart recv thread.") );

 if(inContext->flashContentInRam.appConfig.localServerEnable == true){
   err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Local Server", localTcpServer_thread, 0x200, (void*)inContext );
   require_noerr_action( err, exit, app_log("ERROR: Unable to start the local server thread.") );
 }

 if(inContext->flashContentInRam.appConfig.remoteServerEnable == true){
   err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Remote Client", remoteTcpClient_thread, 0x500, (void*)inContext );
   require_noerr_action( err, exit, app_log("ERROR: Unable to start the remote client thread.") );
 }

exit:
  return err;
}




