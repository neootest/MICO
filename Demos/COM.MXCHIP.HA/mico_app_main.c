#include "platform.h"
#include "mico_define.h"

#include "mico_api.h"
#include "StringUtils.h"
#include "mico_app_define.h"
#include "SocketUtils.h"
#include "haProtocol.h"
#include "PlatformUart.h"

#define app_log(M, ...) custom_log("APP", M, ##__VA_ARGS__)
#define app_log_trace() custom_log_trace("APP")

/* MICO system callback: Restore default configuration provided by application */
void appRestoreDefault_callback(mico_Context_t *inContext)
{
  inContext->flashContentInRam.appConfig.localServerPort = LOCAL_PORT;
  inContext->flashContentInRam.appConfig.localServerEnable = true;
  inContext->flashContentInRam.appConfig.USART_BaudRate = DEFAULT_USART_BaudRate;
  inContext->flashContentInRam.appConfig.remoteServerEnable = true;
  sprintf(inContext->flashContentInRam.appConfig.remoteServerDomain, DEAFULT_REMOTE_SERVER);
  inContext->flashContentInRam.appConfig.remoteServerPort = DEFAULT_REMOTE_SERVER_PORT;
}

OSStatus startMicoApplication( mico_Context_t * const inContext )
{
  app_log_trace();

  OSStatus err = kNoErr;
  require_action(inContext, exit, err = kParamErr);

  haProtocolInit(inContext);
  PlatformUartInitialize(inContext);

  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART Recv", uartRecv_thread, 0x500, (void*)inContext );
  require_noerr_action( err, exit, app_log("ERROR: Unable to start the uart recv thread.") );

 if(inContext->flashContentInRam.appConfig.localServerEnable == true){
   err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Local Server", localTcpServer_thread, 0x1000, (void*)inContext );
   require_noerr_action( err, exit, app_log("ERROR: Unable to start the local server thread.") );
 }

 if(inContext->flashContentInRam.appConfig.remoteServerEnable == true){
   err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Remote Client", remoteTcpClient_thread, 0x500, (void*)inContext );
   require_noerr_action( err, exit, app_log("ERROR: Unable to start the remote client thread.") );
 }

exit:
  return err;
}




