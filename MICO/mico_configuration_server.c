/**
  ******************************************************************************
  * @file    mico_configuration_server.c 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   Local server for mico device configuration 
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

#include "mico_api.h"
#include "mico_define.h"
#include "SocketUtils.h"
#include "HTTPUtils.h"
#include "external/JSON-C/json.h"

#define config_log(M, ...) custom_log("CONFIG SERVER", M, ##__VA_ARGS__)
#define config_log_trace() custom_log_trace("CONFIG SERVER")

#define kEasyLinkURLAuth          "/auth-setup"


static void localConfiglistener_thread(void *inContext);
static void localConfig_thread(void *inFd);
static mico_Context_t *Context;

__weak OSStatus EasyLinkIncommingJsonHTTPMessage( const char *input, mico_Context_t * const inContext )
{
  return kNotPreparedErr;
}
__weak OSStatus EasyLinkCreateReportJsonHTTPMessage( mico_Context_t * const inContext )
{
  return kNotPreparedErr;
}

OSStatus startConfigurationServer ( mico_Context_t * const inContext )
{
  return mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Config Server", localConfiglistener_thread, 0x500, (void*)inContext );
}

void localConfiglistener_thread(void *inContext)
{
  config_log_trace();
  OSStatus err = kUnknownErr;
  int j;
  Context = inContext;
  struct sockaddr_t addr;
  int sockaddr_t_size;
  fd_set readfds;
  char ip_address[16];
  
  int localConfiglistener_fd = -1;

  /*Establish a TCP server fd that accept the tcp clients connections*/ 
  localConfiglistener_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  require_action(IsValidSocket( localConfiglistener_fd ), exit, err = kNoResourcesErr );
  addr.s_ip = INADDR_ANY;
  addr.s_port = CONFIG_SERVICE_PORT;
  err = bind(localConfiglistener_fd, &addr, sizeof(addr));
  require_noerr( err, exit );

  err = listen(localConfiglistener_fd, 0);
  require_noerr( err, exit );

  config_log("Config Server established at port: %d, fd: %d", CONFIG_SERVICE_PORT, localConfiglistener_fd);
  
  while(1){
    FD_ZERO(&readfds);
    FD_SET(localConfiglistener_fd, &readfds);
    select(1, &readfds, NULL, NULL, NULL);

    /*Check tcp connection requests */
    if(FD_ISSET(localConfiglistener_fd, &readfds)){
      sockaddr_t_size = sizeof(struct sockaddr_t);
      j = accept(localConfiglistener_fd, &addr, &sockaddr_t_size);
      if (j > 0) {
        inet_ntoa(ip_address, addr.s_ip );
        config_log("Config Client %s:%d connected, fd: %d", ip_address, addr.s_port, j);
        err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Config Clients", localConfig_thread, 0x500, &j);  
      }
    }
   }

exit:
    config_log("Exit: Local controller exit with err = %d", err);
    mico_rtos_delete_thread(NULL);
    return;
}

void localConfig_thread(void *inFd)
{
  OSStatus err;
  int clientFd = *(int *)inFd;
  uint8_t *inDataBuffer = NULL;
  uint8_t *outDataBuffer = NULL;
  int len;
  fd_set readfds;
  struct timeval_t t;
  const char *json_str;
  uint8_t *httpResponse = NULL;
  size_t httpResponseLen = 0;

  outDataBuffer = malloc(wlanBufferLen);
  require_action(outDataBuffer, exit, err = kNoMemoryErr);

  err = EasyLinkCreateReportJsonHTTPMessage( Context );
  require_noerr( err, exit );

  json_str = json_object_to_json_string(Context->micoStatus.easylink_report);
  require( json_str, exit );

  config_log("Send config object=%s\n", json_str);
  err =  CreateSimpleHTTPMessage_URL( kEasyLinkURLAuth, kMIMEType_JSON, (uint8_t *)json_str, strlen(json_str), &httpResponse, &httpResponseLen );
  require_noerr( err, exit );
  require( httpResponse, exit );

  json_object_put(Context->micoStatus.easylink_report);

  err = SocketSend( clientFd, httpResponse, httpResponseLen );
  free(httpResponse);
  require_noerr( err, exit );
  config_log("Current configuration sent");


  t.tv_sec = 4;
  t.tv_usec = 0;
  
  while(1){

    FD_ZERO(&readfds);
    FD_SET(clientFd, &readfds); 

    select(1, &readfds, NULL, NULL, &t);

    /*Read data from tcp clients and process these data using HA protocol */ 
    if (FD_ISSET(clientFd, &readfds)) {
      len = recv(clientFd, inDataBuffer, wlanBufferLen, 0);
      require_action_quiet(len>0, exit, err = kConnectionErr);
    }
  }

exit:
    config_log("Exit: Client exit with err = %d", err);
    SocketClose(&clientFd);
    if(inDataBuffer) free(inDataBuffer);
    if(outDataBuffer) free(outDataBuffer);
    mico_rtos_delete_thread(NULL);
    return;
}