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

#include "MVDMsgProtocol.h"


#define mvd_log(M, ...) custom_log("MVD", M, ##__VA_ARGS__)
#define mvd_log_trace() custom_log_trace("MVD")

// cloud status
#define DEFAULT_MVD_CLOUD_CONNECTED_MSG_2CLOUD     "{\"MVDCloud\":\"connected\"}"
#define DEFAULT_MVD_CLOUD_CONNECTED_MSG_2MCU       "[MVD]Cloud: connected\r\n"
#define DEFAULT_MVD_CLOUD_DISCONNECTED_MSG_2MCU    "[MVD]Cloud: disconnected\r\n"

// wifi status
#define DEFAULT_MVD_STATION_UP_MSG_2MCU            "[MVD]Wi-Fi: Station up\r\n"
#define DEFAULT_MVD_STATION_DOWN_MSG_2MCU          "[MVD]Wi-Fi: Station down\r\n"

// OTA status
#define DEFAULT_MVD_OTA_CHECK_MSG_2MCU             "[MVD]OTA: Checking ...\r\n"
#define DEFAULT_MVD_OTA_UPDATE_MSG_2MCU            "[MVD]OTA: Update && reboot ...\r\n"
#define DEFAULT_MVD_OTA_UP_TO_DATE_MSG_2MCU        "[MVD]OTA: Up-to-date\r\n"
#define DEFAULT_MVD_OTA_DOWNLOAD_FAILED_MSG_2MCU   "[MVD]OTA: Download failed\r\n"

// dev activate status
#define DEFAULT_MVD_DEV_ACTIVATE_START_MSG_2MCU    "[MVD]Activate: Start ...\r\n"
#define DEFAULT_MVD_DEV_ACTIVATE_OK_MSG_2MCU       "[MVD]Activate: Success\r\n"
#define DEFAULT_MVD_DEV_ACTIVATE_FAILED_MSG_2MCU   "[MVD]Activate: Failed\r\n"

// restore config status
#define DEFAULT_MVD_RESET_CLOUD_INFO_START_MSG_2MCU   "[MVD]CloudReset: Start ...\r\n"
#define DEFAULT_MVD_RESET_CLOUD_INFO_OK_MSG_2MCU      "[MVD]CloudReset: Success\r\n"
#define DEFAULT_MVD_RESET_CLOUD_INFO_FAILED_MSG_2MCU  "[MVD]CloudReset: Failed\r\n"


static mico_semaphore_t _wifi_station_on_sem = NULL;
static mico_semaphore_t _reset_cloud_info_sem = NULL;

void mvdNotify_WifiStatusHandler(WiFiEvent event, mico_Context_t * const inContext)
{
  mvd_log_trace();
  (void)inContext;
  switch (event) {
  case NOTIFY_STATION_UP:
    MVDDevInterfaceSend(DEFAULT_MVD_STATION_UP_MSG_2MCU, 
                        strlen(DEFAULT_MVD_STATION_UP_MSG_2MCU));
    if(NULL == _wifi_station_on_sem){
      mico_rtos_init_semaphore(&_wifi_station_on_sem, 1);
    }
    mico_rtos_set_semaphore(&_wifi_station_on_sem);
    // set LED to green means station down, data: on/off,H,S,B
    LedControlMsgHandler("1,120,100,100", strlen("1,120,100,100"));
    break;
  case NOTIFY_STATION_DOWN:
    MVDDevInterfaceSend(DEFAULT_MVD_STATION_DOWN_MSG_2MCU, 
                        strlen(DEFAULT_MVD_STATION_DOWN_MSG_2MCU));
    // set LED to light green means station down, data: on/off,H,S,B
    LedControlMsgHandler("1,120,100,10", strlen("1,120,100,10"));
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

#define DEVICE_CLOUD_RESET_RETRY_CNT 3
OSStatus easycloud_reset_cloud_info(mico_Context_t * const context)
{
  OSStatus err = kUnknownErr;
  MVDResetRequestData_t devDefaultResetData;
  mico_Context_t *inContext = (mico_Context_t *)context;
  int retry_cnt = 1;
  
  do{
    /* cloud context init */
    err = MVDCloudInterfaceInit(inContext);
    if(kNoErr == err){
      mvd_log("[MVD]Device EasyCloud context init [OK]");
    }
    else{
      mvd_log("[MVD]Device EasyCloud context init [FAILED]");
      retry_cnt++;
      continue;
    }
    
    /* cloud info reset */
    mvd_log("[MVD]Device reset EasyCloud info try[%d] ...", retry_cnt);
    memset((void*)&devDefaultResetData, 0, sizeof(devDefaultResetData));
    strncpy(devDefaultResetData.loginId,
            inContext->flashContentInRam.appConfig.virtualDevConfig.loginId,
            MAX_SIZE_LOGIN_ID);
    strncpy(devDefaultResetData.devPasswd,
            inContext->flashContentInRam.appConfig.virtualDevConfig.devPasswd,
            MAX_SIZE_DEV_PASSWD);
    strncpy(devDefaultResetData.user_token,
            inContext->micoStatus.mac,
            MAX_SIZE_USER_TOKEN);
    err = MVDCloudInterfaceResetCloudDevInfo(inContext, devDefaultResetData);
    if(kNoErr == err){
      mvd_log("[MVD]Device reset EasyCloud info [OK]");
    }
    else{
      mvd_log("[MVD]Device reset EasyCloud info [FAILED]");
      retry_cnt++;
    }
    
  }while((kNoErr != err) && (retry_cnt <= DEVICE_CLOUD_RESET_RETRY_CNT));
  
  return err;
}

void MVDDevCloudInfoResetThread(void *arg)
{
  OSStatus err = kUnknownErr;
  mico_Context_t *inContext = (mico_Context_t *)arg;
  
  // stop EasyCloud service first
  err = MVDCloudInterfaceStop(inContext);
  require_noerr_action( err, exit, mvd_log("ERROR: stop EasyCloud service failed!") );
  
  // cloud reset request
  MVDDevInterfaceSend(DEFAULT_MVD_RESET_CLOUD_INFO_START_MSG_2MCU, 
                      strlen(DEFAULT_MVD_RESET_CLOUD_INFO_START_MSG_2MCU));     
  err = easycloud_reset_cloud_info(inContext);
  if(kNoErr == err){
    inContext->appStatus.virtualDevStatus.isCloudConnected = false;
        
    mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
    inContext->flashContentInRam.appConfig.virtualDevConfig.isActivated = false;
    MICOUpdateConfiguration(inContext);
    mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
    
    //mvd_log("[MVD]MVDDevCloudInfoResetThread: cloud reset success!");
    MVDDevInterfaceSend(DEFAULT_MVD_RESET_CLOUD_INFO_OK_MSG_2MCU, 
                        strlen(DEFAULT_MVD_RESET_CLOUD_INFO_OK_MSG_2MCU));
    
    // send ok semaphore
    mico_rtos_set_semaphore(&_reset_cloud_info_sem);
  }
  
exit:
  if(kNoErr != err){
    mvd_log("[MVD]MVDDevCloudInfoResetThread EXIT: err=%d",err);
    MVDDevInterfaceSend(DEFAULT_MVD_RESET_CLOUD_INFO_FAILED_MSG_2MCU, 
                        strlen(DEFAULT_MVD_RESET_CLOUD_INFO_FAILED_MSG_2MCU));
  }
  mico_rtos_delete_thread(NULL);
  return;
}

void MVDMainThread(void *arg)
{
  OSStatus err = kUnknownErr;
  mico_Context_t *inContext = (mico_Context_t *)arg;
  
  bool connected = false;
  MVDOTARequestData_t devOTARequestData;
  MVDActivateRequestData_t devDefaultActivateData;
  
  mvd_log("MVD main thread start.");
  while(kNoErr != mico_rtos_get_semaphore(&_wifi_station_on_sem, MICO_WAIT_FOREVER));
  
 /* check reset cloud info */
  if((inContext->flashContentInRam.appConfig.virtualDevConfig.needCloudReset) &&
     (inContext->flashContentInRam.appConfig.virtualDevConfig.isActivated)){
       // start a thread to reset device info on EasyCloud
       mico_rtos_init_semaphore(&_reset_cloud_info_sem, 1);
       mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "MVDResetCloudInfo",
                               MVDDevCloudInfoResetThread, 0x800,
                               inContext );
       err = mico_rtos_get_semaphore(&_reset_cloud_info_sem, MICO_WAIT_FOREVER);
       if(kNoErr == err){
         mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
         inContext->flashContentInRam.appConfig.virtualDevConfig.needCloudReset = false;
         inContext->flashContentInRam.appConfig.virtualDevConfig.isActivated = false;
         err = MICOUpdateConfiguration(inContext);
         mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
         mvd_log("MVD Cloud reset success!");
       }
       else{
         mvd_log("MVD Cloud reset failed!");
       }
                
       mvd_log("Press reset button...");
       goto exit; // do nothing after reset, please press reset button.
     }
  
  /* OTA check */
  //mvd_log(DEFAULT_MVD_OTA_CHECK_MSG_2MCU);
  MVDDevInterfaceSend(DEFAULT_MVD_OTA_CHECK_MSG_2MCU, 
                        strlen(DEFAULT_MVD_OTA_CHECK_MSG_2MCU));
  memset((void*)&devOTARequestData, 0, sizeof(devOTARequestData));
  strncpy(devOTARequestData.loginId,
          inContext->flashContentInRam.appConfig.virtualDevConfig.loginId,
          MAX_SIZE_LOGIN_ID);
  strncpy(devOTARequestData.devPasswd,
          inContext->flashContentInRam.appConfig.virtualDevConfig.devPasswd,
          MAX_SIZE_DEV_PASSWD);
  strncpy(devOTARequestData.user_token,
          inContext->micoStatus.mac,
          MAX_SIZE_USER_TOKEN);
  err = MVDCloudInterfaceDevFirmwareUpdate(inContext, devOTARequestData);
  if(kNoErr == err){
    if(inContext->appStatus.virtualDevStatus.RecvRomFileSize > 0){
      //mvd_log(DEFAULT_MVD_OTA_UPDATE_MSG_2MCU);
      MVDDevInterfaceSend(DEFAULT_MVD_OTA_UPDATE_MSG_2MCU, 
                        strlen(DEFAULT_MVD_OTA_UPDATE_MSG_2MCU));
      // set bootloader to reboot && update app fw
      memset(&inContext->flashContentInRam.bootTable, 0, sizeof(boot_table_t));
      inContext->flashContentInRam.bootTable.length = inContext->appStatus.virtualDevStatus.RecvRomFileSize;
      inContext->flashContentInRam.bootTable.start_address = UPDATE_START_ADDRESS;
      inContext->flashContentInRam.bootTable.type = 'A';
      inContext->flashContentInRam.bootTable.upgrade_type = 'U';
      if(inContext->flashContentInRam.micoSystemConfig.configured != allConfigured)
        inContext->flashContentInRam.micoSystemConfig.easyLinkByPass = EASYLINK_SOFT_AP_BYPASS;
      MICOUpdateConfiguration(inContext);
      inContext->micoStatus.sys_state = eState_Software_Reset;
      if(inContext->micoStatus.sys_state_change_sem != NULL );
      mico_rtos_set_semaphore(&inContext->micoStatus.sys_state_change_sem);
      mico_thread_sleep(MICO_WAIT_FOREVER);
    }
    else{
      //mvd_log(DEFAULT_MVD_OTA_UP_TO_DATE_MSG_2MCU);
      MVDDevInterfaceSend(DEFAULT_MVD_OTA_UP_TO_DATE_MSG_2MCU, 
                        strlen(DEFAULT_MVD_OTA_UP_TO_DATE_MSG_2MCU));
    }
  }
  else{
    //mvd_log(DEFAULT_MVD_OTA_DOWNLOAD_FAILED_MSG_2MCU);
    MVDDevInterfaceSend(DEFAULT_MVD_OTA_DOWNLOAD_FAILED_MSG_2MCU, 
                        strlen(DEFAULT_MVD_OTA_DOWNLOAD_FAILED_MSG_2MCU));
  }
  
  /* activate when wifi on */
  while(false == inContext->flashContentInRam.appConfig.virtualDevConfig.isActivated){
    // auto activate, using default login_id/dev_pass/user_token
    //mvd_log(DEFAULT_MVD_DEV_ACTIVATE_START_MSG_2MCU);
    MVDDevInterfaceSend(DEFAULT_MVD_DEV_ACTIVATE_START_MSG_2MCU, 
                        strlen(DEFAULT_MVD_DEV_ACTIVATE_START_MSG_2MCU));
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
      //mvd_log("device activate success!");
      MVDDevInterfaceSend(DEFAULT_MVD_DEV_ACTIVATE_OK_MSG_2MCU, 
                          strlen(DEFAULT_MVD_DEV_ACTIVATE_OK_MSG_2MCU));
    }
    else{
      //mvd_log("device activate failed, err = %d, retry in %d s ...", err, 1);
      MVDDevInterfaceSend(DEFAULT_MVD_DEV_ACTIVATE_FAILED_MSG_2MCU, 
                          strlen(DEFAULT_MVD_DEV_ACTIVATE_FAILED_MSG_2MCU));
    }
    mico_thread_sleep(1);
  }
  mvd_log("[MVD]device already activated.");
  
  /* start EasyCloud service */
  err = MVDCloudInterfaceStart(inContext);
  require_noerr_action(err, exit, 
                       mvd_log("ERROR: MVDCloudInterfaceStart failed!") );
  
  /* loop connect status */
  while(1)
  {
    if(inContext->appStatus.virtualDevStatus.isCloudConnected){
      if (!connected){
        connected = true;
        
        // set LED to blue means cloud connected, data: on/off,H,S,B
        LedControlMsgHandler("1,240,100,100", strlen("1,240,100,100"));
        
        //mvd_log("[MVD]Cloud: connected");
        MVDDevInterfaceSend(DEFAULT_MVD_CLOUD_CONNECTED_MSG_2MCU, 
                            strlen(DEFAULT_MVD_CLOUD_CONNECTED_MSG_2MCU));
        MVDCloudInterfaceSendtoChannel(PUBLISH_TOPIC_CHANNEL_STATUS,
                                       DEFAULT_MVD_CLOUD_CONNECTED_MSG_2CLOUD, 
                                       strlen(DEFAULT_MVD_CLOUD_CONNECTED_MSG_2CLOUD));  
      }
    }
    else{
      if (connected){
        connected = false;
        
        // white means cloud disconnect.
        LedControlMsgHandler("1,0,0,10", strlen("1,0,0,10"));
        mvd_log("[MVD]cloud service disconnected!");
        
        //mvd_log("[MVD]Cloud: disconnected");
        MVDDevInterfaceSend(DEFAULT_MVD_CLOUD_DISCONNECTED_MSG_2MCU, 
                            strlen(DEFAULT_MVD_CLOUD_DISCONNECTED_MSG_2MCU));
      }
    }
    
    mico_thread_sleep(1);
  }
  
exit:
  mvd_log("[MVD]MVDMainThread exit err=%d.", err);
  mico_rtos_delete_thread(NULL);
  return;
}


/*******************************************************************************
 * virtual device interfaces init
 ******************************************************************************/

// reset default value
void MVDRestoreDefault(mico_Context_t* const context)
{
  bool cloud_need_reset = false;
  
  if(context->flashContentInRam.appConfig.virtualDevConfig.isActivated){
    cloud_need_reset = true;
  }
  
  // reset all MVD config params
  memset((void*)&(context->flashContentInRam.appConfig.virtualDevConfig), 
         0, sizeof(virtual_device_config_t));
  
  context->flashContentInRam.appConfig.virtualDevConfig.USART_BaudRate = 115200;
  
  context->flashContentInRam.appConfig.virtualDevConfig.isActivated = false;
  sprintf(context->flashContentInRam.appConfig.virtualDevConfig.deviceId, DEFAULT_DEVICE_ID);
  sprintf(context->flashContentInRam.appConfig.virtualDevConfig.masterDeviceKey, DEFAULT_DEVICE_KEY);
  sprintf(context->flashContentInRam.appConfig.virtualDevConfig.romVersion, DEFAULT_ROM_VERSION);
  
  sprintf(context->flashContentInRam.appConfig.virtualDevConfig.loginId, DEFAULT_LOGIN_ID);
  sprintf(context->flashContentInRam.appConfig.virtualDevConfig.devPasswd, DEFAULT_DEV_PASSWD);
  //sprintf(context->flashContentInRam.appConfig.virtualDevConfig.userToken, context->micoStatus.mac);
  
  // set flag to reset cloud next restart
  if(cloud_need_reset){
    context->flashContentInRam.appConfig.virtualDevConfig.needCloudReset = true;
    context->flashContentInRam.appConfig.virtualDevConfig.isActivated = true;
  }
  else{
    context->flashContentInRam.appConfig.virtualDevConfig.needCloudReset = false;
  }
}

OSStatus MVDInit(mico_Context_t* const inContext)
{
  OSStatus err = kUnknownErr;
  
  //init MVD status
  inContext->appStatus.virtualDevStatus.isCloudConnected = false;
  inContext->appStatus.virtualDevStatus.RecvRomFileSize = 0;
  
  //init MCU connect interface
  //err = MVDDevInterfaceInit(inContext);
  //require_noerr_action(err, exit, 
  //                     mvd_log("ERROR: virtual device mcu interface init failed!") );
  
  //init cloud service interface
  err = MVDCloudInterfaceInit(inContext);
  require_noerr_action(err, exit, 
                       mvd_log("ERROR: virtual device cloud interface init failed!") );
  
  // wifi notify
  err = mico_rtos_init_semaphore(&_wifi_station_on_sem, 1);
  require_noerr_action(err, exit, 
                       mvd_log("ERROR: mico_rtos_init_semaphore (_wifi_station_on_sem) failed!") );
  
  err = MICOAddNotification( mico_notify_WIFI_STATUS_CHANGED, (void *)mvdNotify_WifiStatusHandler );
  require_noerr_action(err, exit, 
                       mvd_log("ERROR: MICOAddNotification (mico_notify_WIFI_STATUS_CHANGED) failed!") );
 
  // start MVD main thread
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
 * MVD message send interface
 ******************************************************************************/

// MVD => MCU
// send to USART
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
// if topic is NULL, send to default topic: device_id/out,
// else send to sub-channel: device_id/out/<topic>
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
 * MVD message exchange protocol
 ******************************************************************************/

// handle cloud msg here, for example: send to USART or echo to cloud
OSStatus MVDCloudMsgProcess(mico_Context_t* context, 
                            const char* topic, const unsigned int topicLen,
                            unsigned char *inBuf, unsigned int inBufLen)
{
  mvd_log_trace();
  OSStatus err = kUnknownErr;
//  char* responseTopic = NULL;
//  unsigned char* responseMsg = NULL;
//  unsigned char* ptr = NULL;
//  int responseMsgLen = 0;
//  
//  /* send to USART */
//  err = MVDDevInterfaceSend(inBuf, inBufLen); // transfer raw data to USART
//  require_noerr_action( err, exit, mvd_log("ERROR: send to MCU error! err=%d", err) );
//  
//  /* echo to cloud */
//  // responseTopic = device_id/out, message = [MAC]msg
//  responseTopic = ECS_str_replace(responseTopic, topic, topicLen, "/in", "/out");
//  responseMsgLen = strlen(context->micoStatus.mac) + 2 + inBufLen;
//  responseMsg = (unsigned char*)malloc(responseMsgLen + 1);
//  memset(responseMsg, 0x00, responseMsgLen);
//  if(NULL == responseMsg){
//    err = kNoMemoryErr;
//    goto exit;
//  }
//  ptr = responseMsg;
//  memcpy(ptr, "[", 1);
//  ptr += 1;
//  memcpy(ptr, (const void*)&(context->micoStatus.mac), strlen(context->micoStatus.mac));
//  ptr += strlen(context->micoStatus.mac);
//  memcpy(ptr, "]", 1);
//  ptr += 1;
//  memcpy(ptr, inBuf, inBufLen);
//  ptr += inBufLen;
//  memcpy(ptr, '\0', 1);
//  err = MVDCloudInterfaceSendto(responseTopic, responseMsg, responseMsgLen);
//  if(NULL != responseTopic){
//    free(responseTopic);
//  }
//  if(NULL != responseMsg){
//      ptr = NULL;
//      free(responseMsg);
//  }
  
  /* LED control */
  unsigned char* usartCmd = NULL;
  unsigned int usartCmdLen = 0;
  
  // translate cloud message to control protocol format
  err = MVDMsgTransformCloud2Device(inBuf, inBufLen, &usartCmd, &usartCmdLen);
  if(NULL != usartCmd){
    free(usartCmd);
    usartCmd = NULL;
    usartCmdLen = 0;
  }

  if (kNoErr == err){
    //err = MVDCloudInterfaceSend(LED_RESP_CMD_VALUE_OK, strlen((const char*)LED_RESP_CMD_VALUE_OK));
    return kNoErr;
  }
  else
  {
    //err = MVDCloudInterfaceSend(LED_RESP_CMD_VALUE_FAILED, strlen((const char*)LED_RESP_CMD_VALUE_FAILED));
    return kRequestErr;
  }
  
//exit:
//  return err;
}

// handle MCU msg here, for example: send to Cloud
OSStatus MVDDeviceMsgProcess(mico_Context_t* const context, 
                             uint8_t *inBuf, unsigned int inBufLen)
{
  mvd_log_trace();
  OSStatus err = kUnknownErr;
  
  err = MVDCloudInterfaceSend(inBuf, inBufLen);  // transfer raw data to cloud
  require_noerr_action( err, exit, mvd_log("ERROR: send to cloud error! err=%d", err) );
  return kNoErr;
  
exit:
  return err;
}