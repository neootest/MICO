/**
******************************************************************************
* @file    MicoVirtualDevice.c
* @author  Eshen Wang
* @version V0.2.0
* @date    21-Nov-2014
* @brief   This file contains the implementations
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

#include <stdio.h>

#include "MICODefine.h"
#include "MICONotificationCenter.h"

#include "MicoVirtualDevice.h"
#include "MVDDeviceInterfaces.h"
#include "MVDCloudInterfaces.h"
#include "EasyCloudUtils.h"


#define mvd_log(M, ...) custom_log("MVD", M, ##__VA_ARGS__)
#define mvd_log_trace() custom_log_trace("MVD")


#define DEFAULT_MVD_CLOUD_CONNECTED_MSG_2CLOUD     "{\"MVDCloud\":\"connected\"}"
#define DEFAULT_MVD_CLOUD_CONNECTED_MSG_2MCU       "[MVD]Cloud connected!\r\n"
#define DEFAULT_MVD_CLOUD_DISCONNECTED_MSG_2MCU    "[MVD]Cloud disconnected!\r\n"

static bool _is_wifi_station_on = false;

void mvdNotify_WifiStatusHandler(WiFiEvent event, mico_Context_t * const inContext)
{
  mvd_log_trace();
  (void)inContext;
  switch (event) {
  case NOTIFY_STATION_UP:
    _is_wifi_station_on = true;
    break;
  case NOTIFY_STATION_DOWN:
    _is_wifi_station_on = false;
    break;
  case NOTIFY_AP_UP:
    break;
  case NOTIFY_AP_DOWN:
    break;
  default:
    break;
  }
  return;
}


void MVDMainThread(void *arg)
{
  OSStatus err = kUnknownErr;
  mico_Context_t *inContext = (mico_Context_t *)arg;
  micoMemInfo_t *memInfo = NULL;
  bool connected = false;
  
#ifdef DEVICE_AUTO_ACTIVATE_ENABLE
  MVDActivateRequestData_t devDefaultActivateData;
#endif
  
  mvd_log("MVD main thread start.");
  
  /* Regisist notifications */
  err = MICOAddNotification( mico_notify_WIFI_STATUS_CHANGED, (void *)mvdNotify_WifiStatusHandler );
  require_noerr( err, exit ); 
      
//  memInfo = mico_memory_info();
//  mvd_log("[MVD]system free mem=%d", memInfo->free_memory);
  
  while(1)
  {
    memInfo = mico_memory_info();
    mvd_log("[MVD]system free mem=%d", memInfo->free_memory);
    
    if(inContext->appStatus.virtualDevStatus.isCloudConnected){
      if (!connected){
        mvd_log("[MVD]cloud service connected!");
        MVDDevInterfaceSend(DEFAULT_MVD_CLOUD_CONNECTED_MSG_2MCU, 
                                   strlen(DEFAULT_MVD_CLOUD_CONNECTED_MSG_2MCU));
        MVDCloudInterfaceSendtoChannel(PUBLISH_TOPIC_CHANNEL_STATUS,
                                     DEFAULT_MVD_CLOUD_CONNECTED_MSG_2CLOUD, 
                                     strlen(DEFAULT_MVD_CLOUD_CONNECTED_MSG_2CLOUD));
        
        connected = true;
      }
    }
    else{
      if (connected){
        connected = false; //recovery value;
        mvd_log("[MVD]cloud service disconnected!");
        MVDDevInterfaceSend(DEFAULT_MVD_CLOUD_DISCONNECTED_MSG_2MCU, 
                            strlen(DEFAULT_MVD_CLOUD_DISCONNECTED_MSG_2MCU));
      }
      
#ifdef DEVICE_AUTO_ACTIVATE_ENABLE
      if((true == _is_wifi_station_on) &&
         (false == inContext->flashContentInRam.appConfig.virtualDevConfig.isActivated)){
        // auto activate, using default login_id/dev_pass/user_token
        mvd_log("auto activate device by MVD...");
        memset((void*)&devDefaultActivateData, 0, sizeof(devDefaultActivateData));
        strncpy(devDefaultActivateData.loginId,
                inContext->flashContentInRam.appConfig.virtualDevConfig.loginId,
                MAX_SIZE_LOGIN_ID);
        strncpy(devDefaultActivateData.devPasswd,
                inContext->flashContentInRam.appConfig.virtualDevConfig.devPasswd,
                MAX_SIZE_DEV_PASSWD);
        strncpy(devDefaultActivateData.user_token,
                inContext->micoStatus.mac,
                MAX_SIZE_USER_TOKEN);
        err = MVDCloudInterfaceDevActivate(inContext, devDefaultActivateData);
        if(kNoErr == err){
          mvd_log("device activate success!");
        }
        else{
          mvd_log("device activate failed, will retry in %ds...", 1);
        }
      }
#endif
    }
    
    mico_thread_sleep(1);
  }
  
exit:
  mvd_log("[MVD]ERROR: exit with err=%d",err);
  return;
}


/*******************************************************************************
 * virtual device interfaces init
 ******************************************************************************/

void MVDRestoreDefault(mico_Context_t* const context)
{
  // clean all config buffer
  memset((void*)&(context->flashContentInRam.appConfig.virtualDevConfig), 
         0, sizeof(virtual_device_config_t));
  
  context->flashContentInRam.appConfig.virtualDevConfig.USART_BaudRate = 115200;
  
  context->flashContentInRam.appConfig.virtualDevConfig.isActivated = false;
  sprintf(context->flashContentInRam.appConfig.virtualDevConfig.deviceId, DEFAULT_DEVICE_ID);
  sprintf(context->flashContentInRam.appConfig.virtualDevConfig.masterDeviceKey, DEFAULT_DEVICE_KEY);
  sprintf(context->flashContentInRam.appConfig.virtualDevConfig.romVersion, DEFAULT_ROM_VERSION);
  
  sprintf(context->flashContentInRam.appConfig.virtualDevConfig.loginId, DEFAULT_LOGIN_ID);
  sprintf(context->flashContentInRam.appConfig.virtualDevConfig.devPasswd, DEFAULT_DEV_PASSWD);
  //sprintf(context->flashContentInRam.appConfig.virtualDevConfig.userToken, DEFAULT_USER_TOKEN);
  sprintf(context->flashContentInRam.appConfig.virtualDevConfig.userToken, context->micoStatus.mac);
}

OSStatus MVDInit(mico_Context_t* const inContext)
{
  OSStatus err = kUnknownErr;
  
  //init MVD status
  inContext->appStatus.virtualDevStatus.isCloudConnected = false;
  inContext->appStatus.virtualDevStatus.RecvRomFileSize = 0;
  
  //init MCU connect interface
  err = MVDDevInterfaceInit(inContext);
  require_noerr_action(err, exit, 
                       mvd_log("ERROR: virtual device mcu interface init failed!") );
  
  //init cloud service interface
  err = MVDCloudInterfaceInit(inContext);
  require_noerr_action(err, exit, 
                       mvd_log("ERROR: virtual device cloud interface init failed!") );
  
  // start MVD monitor thread
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "MVD main", 
                                MVDMainThread, STACK_SIZE_MVD_MAIN_THREAD, 
                                inContext );
  
exit:
  return err;
}

/*******************************************************************************
 * MVD get state
 ******************************************************************************/
// cloud connect state
bool MVDCloudIsConnect(mico_Context_t* const context)
{
  if(NULL == context){
    return false;
  }
  return context->appStatus.virtualDevStatus.isCloudConnected;
}

// device activate state
bool MVDIsActivated(mico_Context_t* const context)
{
  if(NULL == context){
    return false;
  }
  return context->flashContentInRam.appConfig.virtualDevConfig.isActivated;
}

/*******************************************************************************
 * MVD message exchange protocol
 ******************************************************************************/

// Cloud => MCU
OSStatus MVDCloudMsgProcess(mico_Context_t* context, 
                            const char* topic, const unsigned int topicLen,
                            unsigned char *inBuf, unsigned int inBufLen)
{
  mvd_log_trace();
  OSStatus err = kUnknownErr;
  char* responseTopic = NULL;
  unsigned char* responseMsg = NULL;
  unsigned char* ptr = NULL;
  int responseMsgLen = 0;
  
  err = MVDDevInterfaceSend(inBuf, inBufLen); // transfer raw data
  require_noerr_action( err, exit, mvd_log("ERROR: send to MCU error! err=%d", err) );
  
  // add response to cloud(echo), replace topic 'device_id/in/xxx' to 'device_id/out/xxx'
  responseTopic = str_replace(responseTopic, topic, topicLen, "/in", "/out");
  responseMsgLen = strlen(context->micoStatus.mac) + 2 + inBufLen;
  responseMsg = (unsigned char*)malloc(responseMsgLen + 1);
  memset(responseMsg, 0x00, responseMsgLen);
  if(NULL == responseMsg){
    err = kNoMemoryErr;
    goto exit;
  }
  ptr = responseMsg;
  memcpy(ptr, "[", 1);
  ptr += 1;
  memcpy(ptr, (const void*)&(context->micoStatus.mac), strlen(context->micoStatus.mac));
  ptr += strlen(context->micoStatus.mac);
  memcpy(ptr, "]", 1);
  ptr += 1;
  memcpy(ptr, inBuf, inBufLen);
  ptr += inBufLen;
  memcpy(ptr, '\0', 1);
  err = MVDCloudInterfaceSendto(responseTopic, responseMsg, responseMsgLen);
  if(NULL != responseTopic){
    free(responseTopic);
  }
  if(NULL != responseMsg){
      ptr = NULL;
      free(responseMsg);
  }
  
  return kNoErr;
  
exit:
  return err;
}

// MCU => Cloud
OSStatus MVDDeviceMsgProcess(mico_Context_t* const context, 
                             uint8_t *inBuf, unsigned int inBufLen)
{
  mvd_log_trace();
  OSStatus err = kUnknownErr;
  
  err = MVDCloudInterfaceSend(inBuf, inBufLen);  // transfer raw data
  require_noerr_action( err, exit, mvd_log("ERROR: send to cloud error! err=%d", err) );
  return kNoErr;
  
exit:
  return err;
}


/*******************************************************************************
 * MVD message send interface
 ******************************************************************************/

// MVD => MCU
OSStatus MVDSendMsg2Device(mico_Context_t* const context, 
                           unsigned char *inBuf, unsigned int inBufLen)
{
  mvd_log_trace();
  OSStatus err = kUnknownErr;
  
  err = MVDDevInterfaceSend(inBuf, inBufLen);  // transfer raw data
  require_noerr_action( err, exit, mvd_log("ERROR: send to cloud error! err=%d", err) );
  return kNoErr;
  
exit:
  return err;
}

// MVD => Cloud
OSStatus MVDSendMsg2Cloud(mico_Context_t* const context, const char* topic, 
                       unsigned char *inBuf, unsigned int inBufLen)
{
  mvd_log_trace();
  OSStatus err = kUnknownErr;
  
  err = MVDCloudInterfaceSendtoChannel(topic, inBuf, inBufLen);  // transfer raw data
  require_noerr_action( err, exit, mvd_log("ERROR: send to cloud error! err=%d", err) );
  return kNoErr;
  
exit:
  return err;
}


/*******************************************************************************
 * MVD cloud control interfaces
 ******************************************************************************/

//activate
OSStatus MVDActivate(mico_Context_t* const context, 
                     MVDActivateRequestData_t activateData)
{
  OSStatus err = kUnknownErr;
  
  err = MVDCloudInterfaceDevActivate(context, activateData);
  require_noerr_action(err, exit, 
                       mvd_log("ERROR: device activate failed! err=%d", err) );
  return kNoErr;
  
exit:
  return err;
}
  
//authorize
OSStatus MVDAuthorize(mico_Context_t* const context,
                      MVDAuthorizeRequestData_t authorizeData)
{
  OSStatus err = kUnknownErr;
  mico_Context_t *inContext = context;

  err = MVDCloudInterfaceDevAuthorize(inContext, authorizeData);
  require_noerr_action(err, exit, 
                       mvd_log("ERROR: device authorize failed! err=%d", err) );
  return kNoErr;
  
exit:
  return err;
}

//OTA
OSStatus MVDFirmwareUpdate(mico_Context_t* const context,
                           MVDOTARequestData_t OTAData)
{
  OSStatus err = kUnknownErr;
  mico_Context_t *inContext = context;
  
  err = MVDCloudInterfaceDevFirmwareUpdate(inContext, OTAData);
  require_noerr_action(err, exit, 
                       mvd_log("ERROR: Firmware Update error! err=%d", err) );
  return kNoErr;
  
exit:
  return err;
}

//reset device info on cloud
OSStatus MVDResetCloudDevInfo(mico_Context_t* const context,
                              MVDResetRequestData_t devResetData)
{
  OSStatus err = kUnknownErr;
  mico_Context_t *inContext = context;
  
  err = MVDCloudInterfaceResetCloudDevInfo(inContext, devResetData);
  require_noerr_action(err, exit, 
                       mvd_log("ERROR: reset device info on cloud error! err=%d", err) );
  return kNoErr;
  
exit:
  return err;
}

