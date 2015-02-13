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

#include "EasyCloudUtils.h"
#include "MicoVirtualDevice.h"


#define config_log(M, ...) custom_log("CONFIG SERVER", M, ##__VA_ARGS__)
#define config_log_trace() custom_log_trace("CONFIG SERVER")

#define STACK_SIZE_MVD_CONFIG_SERVER_THREAD   0x300
#define STACK_SIZE_MVD_CONFIG_CLIENT_THREAD   0x800

#define kCONFIGURLRead    "/config-read"
#define kCONFIGURLWrite   "/config-write"
#define kCONFIGURLOTA     "/OTA"

//for temp config by WES at 20141123
#define kCONFIGURLDevState             "/dev-state"
#define kCONFIGURLDevActivate          "/dev-activate"
#define kCONFIGURLDevAuthorize         "/dev-authorize"
#define kCONFIGURLResetCloudDevInfo    "/dev-cloud_reset"
#define kCONFIGURLDevFWUpdate          "/dev-fw_update"

extern OSStatus     ConfigIncommingJsonMessage( const char *input, mico_Context_t * const inContext );
extern json_object* ConfigCreateReportJsonMessage( mico_Context_t * const inContext );
extern OSStatus getMVDActivateRequestData(const char *input, MVDActivateRequestData_t *activateData);
extern OSStatus getMVDAuthorizeRequestData(const char *input, MVDAuthorizeRequestData_t *authorizeData);
extern OSStatus getMVDResetRequestData(const char *input, MVDResetRequestData_t *devResetData);
extern OSStatus getMVDOTARequestData(const char *input, MVDOTARequestData_t *OTAData);
extern OSStatus getMVDGetStateRequestData(const char *input, MVDGetStateRequestData_t *devGetStateData);

static void localConfiglistener_thread(void *inContext);
static void localConfig_thread(void *inFd);
static mico_Context_t *Context;
static OSStatus _LocalConfigRespondInComingMessage(int fd, ECS_HTTPHeader_t* inHeader, mico_Context_t * const inContext);

OSStatus MICOStartConfigServer ( mico_Context_t * const inContext )
{
  return mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Config Server", localConfiglistener_thread, STACK_SIZE_MVD_CONFIG_SERVER_THREAD, (void*)inContext );
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
        err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Config Clients", localConfig_thread, STACK_SIZE_MVD_CONFIG_CLIENT_THREAD, &j);  
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
  ECS_HTTPHeader_t *httpHeader = NULL;

  config_log_trace();
  httpHeader = ECS_HTTPHeaderCreate();
  require_action( httpHeader, exit, err = kNoMemoryErr );
  ECS_HTTPHeaderClear( httpHeader );

  t.tv_sec = 60;
  t.tv_usec = 0;

  while(1){
    FD_ZERO(&readfds);
    FD_SET(clientFd, &readfds);
    clientFdIsSet = 0;

    if(httpHeader->len == 0){
      err = select(1, &readfds, NULL, NULL, &t);
      clientFdIsSet = FD_ISSET(clientFd, &readfds);
    }
  
    if(clientFdIsSet||httpHeader->len){
      err = ECS_SocketReadHTTPHeader( clientFd, httpHeader );

      switch ( err )
      {
        case kNoErr:
          // Read the rest of the HTTP body if necessary
          do{
            err = ECS_SocketReadHTTPBody( clientFd, httpHeader );
            require_noerr(err, exit);

            // Call the HTTPServer owner back with the acquired HTTP header
            err = _LocalConfigRespondInComingMessage( clientFd, httpHeader, Context );
            //require_noerr( err, exit ); 
            if(kConnectionErr == err){
              goto exit; // connect err mean client closed.
            }
            if(httpHeader->contentLength == 0)
              break;
          } while( httpHeader->chunkedData == true || httpHeader->dataEndedbyClose == true);
      
          // Reuse HTTPHeader
          ECS_HTTPHeaderClear( httpHeader );
        break;

        case EWOULDBLOCK:
            // NO-OP, keep reading
        break;

        case kNoSpaceErr:
          config_log("ERROR: Cannot fit HTTPHeader.");
          goto exit;
        break;

        case kConnectionErr:
          // NOTE: kConnectionErr from ECS_SocketReadHTTPHeader means it's closed
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
  if(kConnectionErr != err){
    config_log("Exit: Client exit with err = %d", err);
  }
  SocketClose(&clientFd);
  ECS_HTTPHeaderClear( httpHeader );
  if(httpHeader) free(httpHeader);
  mico_rtos_delete_thread(NULL);
  return;
}


OSStatus _LocalConfigRespondInComingMessage(int fd, ECS_HTTPHeader_t* inHeader, mico_Context_t * const inContext)
{
  OSStatus err = kUnknownErr;
  const char *  json_str;
  uint8_t *httpResponse = NULL;
  size_t httpResponseLen = 0;
  json_object* report = NULL;
  config_log_trace();
  
  MVDActivateRequestData_t devActivateRequestData;
  MVDAuthorizeRequestData_t devAuthorizeRequestData;
  MVDResetRequestData_t devResetRequestData;
  MVDOTARequestData_t devOTARequestData;
  MVDGetStateRequestData_t devGetStateRequestData;

#if 1
  /* This is a demo code for http package has chunked data */
  char *tempStr;
  if(inHeader->chunkedData == true){
    tempStr = calloc(inHeader->contentLength+1, sizeof(uint8_t));
    memcpy(tempStr, inHeader->extraDataPtr, inHeader->contentLength);
    config_log("Recv==>%s", tempStr);
    free(tempStr);
    return kNoErr;
  }
#endif

  //config_log("recv=%s", inHeader->buf);
  if(ECS_HTTPHeaderMatchURL( inHeader, kCONFIGURLRead ) == kNoErr){    
    report = ConfigCreateReportJsonMessage( inContext );
    require( report, exit );
    json_str = json_object_to_json_string(report);
    require_action( json_str, exit, err = kNoMemoryErr );
    config_log("Send config object=%s", json_str);
    err =  ECS_CreateSimpleHTTPMessageNoCopy( ECS_kMIMEType_JSON, strlen(json_str), &httpResponse, &httpResponseLen );
    require_noerr( err, exit );
    require( httpResponse, exit );
    err = SocketSend( fd, httpResponse, httpResponseLen );
    require_noerr( err, exit );
    err = SocketSend( fd, (uint8_t *)json_str, strlen(json_str) );
    require_noerr( err, exit );
    config_log("Current configuration sent");
    SocketClose(&fd);
    err = kConnectionErr; //Return an err to close the current thread
    goto exit;
  }
  else if(ECS_HTTPHeaderMatchURL( inHeader, kCONFIGURLWrite ) == kNoErr){
    if(inHeader->contentLength > 0){
      config_log("Recv new configuration, apply and reset");
      err = ConfigIncommingJsonMessage( inHeader->extraDataPtr, inContext);
      require_noerr( err, exit );
      err =  ECS_CreateSimpleHTTPOKMessage( &httpResponse, &httpResponseLen );
      require_noerr( err, exit );
      require( httpResponse, exit );
      err = SocketSend( fd, httpResponse, httpResponseLen );
      SocketClose(&fd);
      
      inContext->micoStatus.sys_state = eState_Software_Reset;
      require(inContext->micoStatus.sys_state_change_sem, exit);
      mico_rtos_set_semaphore(&inContext->micoStatus.sys_state_change_sem);
    }
    goto exit;
  }
  else if(ECS_HTTPHeaderMatchURL( inHeader, kCONFIGURLDevState ) == kNoErr){
    if(inHeader->contentLength > 0){
      config_log("Recv device getState request.");
      memset((void*)&devGetStateRequestData, '\0', sizeof(devGetStateRequestData));
      err = getMVDGetStateRequestData(inHeader->extraDataPtr, &devGetStateRequestData);
      require_noerr( err, exit );
      report = json_object_new_object();
      err = MVDGetState(inContext, devGetStateRequestData, report);
      require_noerr( err, exit );
      config_log("get device state success!");
      
      json_str = (char*)json_object_to_json_string(report);
      //config_log("json_str=%s", json_str);
      
      err =  ECS_CreateSimpleHTTPMessage( ECS_kMIMEType_JSON, (uint8_t*)json_str, strlen(json_str), 
                                     &httpResponse, &httpResponseLen );
      require( httpResponse, exit );
      err = SocketSend( fd, httpResponse, httpResponseLen );
      SocketClose(&fd);
      
      err = kConnectionErr; //Return an err to close the current thread
    }
    goto exit;
  }
  else if(ECS_HTTPHeaderMatchURL( inHeader, kCONFIGURLDevActivate ) == kNoErr){
    if(inHeader->contentLength > 0){
      config_log("Recv device activate request.");
      memset((void*)&devActivateRequestData, '\0', sizeof(devActivateRequestData));
      err = getMVDActivateRequestData(inHeader->extraDataPtr, &devActivateRequestData);
      require_noerr( err, exit );
      
      err = MVDActivate(inContext, devActivateRequestData);
      require_noerr( err, exit );
      config_log("Device activate success!");
      
      report = json_object_new_object();
      require_action(report, exit, err = kNoMemoryErr);
      json_object_object_add(report, "device_id",
                             json_object_new_string(inContext->flashContentInRam.appConfig.virtualDevConfig.deviceId)); 
      
      json_str = (char*)json_object_to_json_string(report);
      //config_log("json_str=%s", json_str);
      
      err =  ECS_CreateSimpleHTTPMessage( ECS_kMIMEType_JSON, (uint8_t*)json_str, strlen(json_str), 
                                     &httpResponse, &httpResponseLen );
      require_noerr( err, exit );
      require( httpResponse, exit );
      err = SocketSend( fd, httpResponse, httpResponseLen );
      SocketClose(&fd);
      
      inContext->micoStatus.sys_state = eState_Software_Reset;
      require(inContext->micoStatus.sys_state_change_sem, exit);
      mico_rtos_set_semaphore(&inContext->micoStatus.sys_state_change_sem);
    }
    goto exit;
  }
  else if(ECS_HTTPHeaderMatchURL( inHeader, kCONFIGURLDevAuthorize ) == kNoErr){
    if(inHeader->contentLength > 0){
      config_log("Recv device authorize request.");
      memset((void*)&devAuthorizeRequestData, '\0', sizeof(devAuthorizeRequestData));
      err = getMVDAuthorizeRequestData( inHeader->extraDataPtr, &devAuthorizeRequestData);
      require_noerr( err, exit );
      
      err = MVDAuthorize(inContext, devAuthorizeRequestData);
      require_noerr( err, exit );
      config_log("Device authorize success!");
      
      report = json_object_new_object();
      require_action(report, exit, err = kNoMemoryErr);
      json_object_object_add(report, "device_id",
                             json_object_new_string(inContext->flashContentInRam.appConfig.virtualDevConfig.deviceId)); 
      
      json_str = (char*)json_object_to_json_string(report);
      //config_log("json_str=%s", json_str);
      
      err =  ECS_CreateSimpleHTTPMessage( ECS_kMIMEType_JSON, (uint8_t*)json_str, strlen(json_str), 
                                     &httpResponse, &httpResponseLen );
      require( httpResponse, exit );
      err = SocketSend( fd, httpResponse, httpResponseLen );
      SocketClose(&fd);
      
      err = kConnectionErr; //Return an err to close the current thread
      
//      inContext->micoStatus.sys_state = eState_Software_Reset;
//      require(inContext->micoStatus.sys_state_change_sem, exit);
//      mico_rtos_set_semaphore(&inContext->micoStatus.sys_state_change_sem);
    }
    goto exit;
  }
  else if(ECS_HTTPHeaderMatchURL( inHeader, kCONFIGURLResetCloudDevInfo ) == kNoErr){
    if(inHeader->contentLength > 0){
      config_log("Recv cloud device info reset request.");
      memset((void*)&devResetRequestData, '\0', sizeof(devResetRequestData));
      err = getMVDResetRequestData( inHeader->extraDataPtr, &devResetRequestData);
      require_noerr( err, exit );
      
      err = MVDResetCloudDevInfo(inContext, devResetRequestData);
      require_noerr( err, exit );
      config_log("Device cloud reset success!");
      
      err =  ECS_CreateSimpleHTTPOKMessage( &httpResponse, &httpResponseLen );
      require_noerr( err, exit );
      require( httpResponse, exit );
      err = SocketSend( fd, httpResponse, httpResponseLen );
      SocketClose(&fd);
      
      inContext->micoStatus.sys_state = eState_Software_Reset;
      require(inContext->micoStatus.sys_state_change_sem, exit);
      mico_rtos_set_semaphore(&inContext->micoStatus.sys_state_change_sem);
    }
    goto exit;
  }
#ifdef MICO_FLASH_FOR_UPDATE
  else if(ECS_HTTPHeaderMatchURL( inHeader, kCONFIGURLDevFWUpdate ) == kNoErr){
    if(inHeader->contentLength > 0){
      config_log("Recv device fw_update request.");
      memset((void*)&devOTARequestData, '\0', sizeof(devOTARequestData));
      err = getMVDOTARequestData( inHeader->extraDataPtr, &devOTARequestData);
      require_noerr( err, exit );
      
      err = MVDFirmwareUpdate(inContext, devOTARequestData);
      require_noerr( err, exit );
      config_log("Device firmware update success!");
      
      err =  ECS_CreateSimpleHTTPOKMessage( &httpResponse, &httpResponseLen );
      require_noerr( err, exit );
      require( httpResponse, exit );
      err = SocketSend( fd, httpResponse, httpResponseLen );
      SocketClose(&fd);
      
      config_log("OTA bin_size=%lld, bin_version=%s", 
                 inContext->appStatus.virtualDevStatus.RecvRomFileSize,
                 inContext->flashContentInRam.appConfig.virtualDevConfig.romVersion );
      if(0 == inContext->appStatus.virtualDevStatus.RecvRomFileSize){
        //no need to update, return size = 0, no need to boot bootloader
        err = kNoErr;
        goto exit;
      }
      
      mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
      memset(&inContext->flashContentInRam.bootTable, 0, sizeof(boot_table_t));
      inContext->flashContentInRam.bootTable.length = inContext->appStatus.virtualDevStatus.RecvRomFileSize;
      inContext->flashContentInRam.bootTable.start_address = UPDATE_START_ADDRESS;
      inContext->flashContentInRam.bootTable.type = 'A';
      inContext->flashContentInRam.bootTable.upgrade_type = 'U';
      MICOUpdateConfiguration(inContext);
      mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
      
      inContext->micoStatus.sys_state = eState_Software_Reset;
      require(inContext->micoStatus.sys_state_change_sem, exit);
      mico_rtos_set_semaphore(&inContext->micoStatus.sys_state_change_sem);
    }
      
    goto exit;
  }
  else if(ECS_HTTPHeaderMatchURL( inHeader, kCONFIGURLOTA ) == kNoErr){
    if(inHeader->contentLength > 0){
      config_log("Receive OTA data!");
      mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
      memset(&inContext->flashContentInRam.bootTable, 0, sizeof(boot_table_t));
      inContext->flashContentInRam.bootTable.length = inHeader->contentLength;
      inContext->flashContentInRam.bootTable.start_address = UPDATE_START_ADDRESS;
      inContext->flashContentInRam.bootTable.type = 'A';
      inContext->flashContentInRam.bootTable.upgrade_type = 'U';
      MICOUpdateConfiguration(inContext);
      mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
      SocketClose(&fd);
      inContext->micoStatus.sys_state = eState_Software_Reset;
      require(inContext->micoStatus.sys_state_change_sem, exit);
      mico_rtos_set_semaphore(&inContext->micoStatus.sys_state_change_sem);
    }
    goto exit;
  }
#endif
  else{
    return kNotFoundErr;
  };

exit:
  if((kNoErr != err) && (fd > 0)){
    ECS_CreateSimpleHTTPFailedMessage( &httpResponse, &httpResponseLen );
    //require_noerr( err, exit );  // keep previous err num
    require( httpResponse, exit );
    SocketSend( fd, httpResponse, httpResponseLen );
    SocketClose(&fd);
  }
      
  if(httpResponse)  free(httpResponse);
  if(report)        json_object_put(report);

  return err;
}
