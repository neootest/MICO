/**
******************************************************************************
* @file    MICOConfigServer.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   Local TCP server for mico device configuration 
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

#include "MICO.h"
#include "MICODefine.h"
#include "SocketUtils.h"
#include "Platform.h"
#include "Platform_common_config.h"
#include "HTTPUtils.h"
#include "MICONotificationCenter.h"

#define config_log(M, ...) custom_log("CONFIG SERVER", M, ##__VA_ARGS__)
#define config_log_trace() custom_log_trace("CONFIG SERVER")

#define kCONFIGURLRead          "/config-read"
#define kCONFIGURLWrite         "/config-write"
#define kCONFIGURLWriteByUAP    "/config-write-uap"  /* Don't reboot but connect to AP immediately */
#define kCONFIGURLOTA           "/OTA"

extern OSStatus     ConfigIncommingJsonMessage( const char *input, mico_Context_t * const inContext );
extern OSStatus     ConfigIncommingJsonMessageUAP( const char *input, mico_Context_t * const inContext );
extern json_object* ConfigCreateReportJsonMessage( mico_Context_t * const inContext );

static void localConfiglistener_thread(void *inContext);
static void localConfig_thread(void *inFd);
static mico_Context_t *Context;
static OSStatus _LocalConfigRespondInComingMessage(int fd, HTTPHeader_t* inHeader, mico_Context_t * const inContext);
static void _easylinkConnectWiFi( mico_Context_t * const inContext);


OSStatus MICOStartConfigServer ( mico_Context_t * const inContext )
{
  return mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Config Server", localConfiglistener_thread, STACK_SIZE_LOCAL_CONFIG_SERVER_THREAD, (void*)inContext );
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
        err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Config Clients", localConfig_thread, STACK_SIZE_LOCAL_CONFIG_CLIENT_THREAD, &j);  
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
  int clientFdIsSet;
  fd_set readfds;
  struct timeval_t t;
  HTTPHeader_t *httpHeader = NULL;

  config_log_trace();
  httpHeader = HTTPHeaderCreate();
  require_action( httpHeader, exit, err = kNoMemoryErr );
  HTTPHeaderClear( httpHeader );

  t.tv_sec = 60;
  t.tv_usec = 0;
  config_log("Free memory %d bytes", MicoGetMemoryInfo()->free_memory) ; 

  while(1){
    FD_ZERO(&readfds);
    FD_SET(clientFd, &readfds);
    clientFdIsSet = 0;

    if(httpHeader->len == 0){
      require(select(1, &readfds, NULL, NULL, &t) >= 0, exit);
      clientFdIsSet = FD_ISSET(clientFd, &readfds);
    }
  
    if(clientFdIsSet||httpHeader->len){
      mico_rtos_lock_mutex(&Context->flashContentInRam_mutex);
      err = SocketReadHTTPHeader( clientFd, httpHeader );
      if( err != kNoErr) mico_rtos_unlock_mutex(&Context->flashContentInRam_mutex);

      switch ( err )
      {
        case kNoErr:
          // Read the rest of the HTTP body if necessary
          do{
            err = SocketReadHTTPBody( clientFd, httpHeader );
            mico_rtos_unlock_mutex(&Context->flashContentInRam_mutex);
            require_noerr(err, exit);

            // Call the HTTPServer owner back with the acquired HTTP header
            err = _LocalConfigRespondInComingMessage( clientFd, httpHeader, Context );
            require_noerr( err, exit ); 
            if(httpHeader->contentLength == 0)
              break;
          } while( httpHeader->chunkedData == true || httpHeader->dataEndedbyClose == true);
      
          // Reuse HTTPHeader
          HTTPHeaderClear( httpHeader );
        break;

        case EWOULDBLOCK:
            // NO-OP, keep reading
        break;

        case kNoSpaceErr:
          config_log("ERROR: Cannot fit HTTPHeader.");
          goto exit;
        break;

        case kConnectionErr:
          // NOTE: kConnectionErr from SocketReadHTTPHeader means it's closed
          config_log("ERROR: Connection closed.");
          goto exit;
           //goto Reconn;
        break;
        default:
          config_log("ERROR: HTTP Header parse internal error: %d", err);
          goto exit;
      }
    }
  }

exit:
  config_log("Exit: Client exit with err = %d", err);
  SocketClose(&clientFd);
  if(httpHeader) free(httpHeader);
  mico_rtos_delete_thread(NULL);
  return;
}


OSStatus _LocalConfigRespondInComingMessage(int fd, HTTPHeader_t* inHeader, mico_Context_t * const inContext)
{
  OSStatus err = kUnknownErr;
  const char *  json_str;
  uint8_t *httpResponse = NULL;
  size_t httpResponseLen = 0;
  json_object* report = NULL;
  config_log_trace();

  if(HTTPHeaderMatchURL( inHeader, kCONFIGURLRead ) == kNoErr){    
    report = ConfigCreateReportJsonMessage( inContext );
    require( report, exit );
    json_str = json_object_to_json_string(report);
    require_action( json_str, exit, err = kNoMemoryErr );
    config_log("Send config object=%s", json_str);
    err =  CreateSimpleHTTPMessageNoCopy( kMIMEType_JSON, strlen(json_str), &httpResponse, &httpResponseLen );
    require_noerr( err, exit );
    require( httpResponse, exit );
    err = SocketSend( fd, httpResponse, httpResponseLen );
    require_noerr( err, exit );
    err = SocketSend( fd, (uint8_t *)json_str, strlen(json_str) );
    require_noerr( err, exit );
    config_log("Current configuration sent");
    goto exit;
  }
  else if(HTTPHeaderMatchURL( inHeader, kCONFIGURLWrite ) == kNoErr){
    if(inHeader->contentLength > 0){
      config_log("Recv new configuration, apply and reset");
      err = ConfigIncommingJsonMessage( inHeader->extraDataPtr, inContext);
      require_noerr( err, exit );
      inContext->flashContentInRam.micoSystemConfig.configured = allConfigured;
      MICOUpdateConfiguration(inContext);

      err =  CreateSimpleHTTPOKMessage( &httpResponse, &httpResponseLen );
      require_noerr( err, exit );
      require( httpResponse, exit );
      err = SocketSend( fd, httpResponse, httpResponseLen );
      SocketClose(&fd);
      inContext->micoStatus.sys_state = eState_Software_Reset;
      if(inContext->micoStatus.sys_state_change_sem != NULL );
        mico_rtos_set_semaphore(&inContext->micoStatus.sys_state_change_sem);
      mico_thread_sleep(MICO_WAIT_FOREVER);
    }
    goto exit;
  }
else if(HTTPHeaderMatchURL( inHeader, kCONFIGURLWriteByUAP ) == kNoErr){
    if(inHeader->contentLength > 0){
      config_log("Recv new configuration from uAP, apply and connect to AP");
      err = ConfigIncommingJsonMessageUAP( inHeader->extraDataPtr, inContext);
      require_noerr( err, exit );
      MICOUpdateConfiguration(inContext);

      err =  CreateSimpleHTTPOKMessage( &httpResponse, &httpResponseLen );
      require_noerr( err, exit );
      require( httpResponse, exit );

      err = SocketSend( fd, httpResponse, httpResponseLen );
      require_noerr( err, exit );
      sleep(1);

      micoWlanSuspendSoftAP();
      _easylinkConnectWiFi( inContext );

      err = kConnectionErr; //Return an err to close socket and exit the current thread
    }
    goto exit;
  }
#ifdef MICO_FLASH_FOR_UPDATE
  else if(HTTPHeaderMatchURL( inHeader, kCONFIGURLOTA ) == kNoErr){
    if(inHeader->contentLength > 0){
      config_log("Receive OTA data!");
      mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
      memset(&inContext->flashContentInRam.bootTable, 0, sizeof(boot_table_t));
      inContext->flashContentInRam.bootTable.length = inHeader->contentLength;
      inContext->flashContentInRam.bootTable.start_address = UPDATE_START_ADDRESS;
      inContext->flashContentInRam.bootTable.type = 'A';
      inContext->flashContentInRam.bootTable.upgrade_type = 'U';
      if(inContext->flashContentInRam.micoSystemConfig.configured != allConfigured)
        inContext->flashContentInRam.micoSystemConfig.easyLinkByPass = EASYLINK_SOFT_AP_BYPASS;
      MICOUpdateConfiguration(inContext);
      mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
      SocketClose(&fd);
      inContext->micoStatus.sys_state = eState_Software_Reset;
      if(inContext->micoStatus.sys_state_change_sem != NULL );
        mico_rtos_set_semaphore(&inContext->micoStatus.sys_state_change_sem);
      mico_thread_sleep(MICO_WAIT_FOREVER);
    }
    goto exit;
  }
#endif
  else{
    return kNotFoundErr;
  };

 exit:
  if(inHeader->persistent == false)  //Return an err to close socket and exit the current thread
    err = kConnectionErr;
  if(httpResponse)  free(httpResponse);
  if(report)        json_object_put(report);

  return err;

}

static void _easylinkConnectWiFi( mico_Context_t * const inContext)
{
  config_log_trace();
  network_InitTypeDef_adv_st wNetConfig;
  memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_adv_st));
  
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  strncpy((char*)wNetConfig.ap_info.ssid, inContext->flashContentInRam.micoSystemConfig.ssid, maxSsidLen);
  wNetConfig.ap_info.security = SECURITY_TYPE_AUTO;
  memcpy(wNetConfig.key, inContext->flashContentInRam.micoSystemConfig.user_key, maxKeyLen);
  wNetConfig.key_len = inContext->flashContentInRam.micoSystemConfig.user_keyLength;
  wNetConfig.dhcpMode = inContext->flashContentInRam.micoSystemConfig.dhcpEnable;
  strncpy((char*)wNetConfig.local_ip_addr, inContext->flashContentInRam.micoSystemConfig.localIp, maxIpLen);
  strncpy((char*)wNetConfig.net_mask, inContext->flashContentInRam.micoSystemConfig.netMask, maxIpLen);
  strncpy((char*)wNetConfig.gateway_ip_addr, inContext->flashContentInRam.micoSystemConfig.gateWay, maxIpLen);
  strncpy((char*)wNetConfig.dnsServer_ip_addr, inContext->flashContentInRam.micoSystemConfig.dnsServer, maxIpLen);

  wNetConfig.wifi_retry_interval = 100;
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
  micoWlanStartAdv(&wNetConfig);
  config_log("connect to %s.....", wNetConfig.ap_info.ssid);
}



