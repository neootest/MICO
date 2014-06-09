/**
  ******************************************************************************
  * @file    EasyLink.c 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   This file provide the easylink function and FTC server for quick 
  *          provisioning and first time configuration.
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
#include "MICONotificationCenter.h"

#include "Platform.h"
#include "PlatformFlash.h"
#include "StringUtils.h"
#include "HTTPUtils.h"
#include "SocketUtils.h"

#include "EasyLink.h"
  
// EasyLink HTTP messages
#define kEasyLinkURLAuth          "/auth-setup"

#define easylink_log(M, ...) custom_log("EasyLink", M, ##__VA_ARGS__)
#define easylink_log_trace() custom_log_trace("EasyLink")

static void easylink_thread(void *inContext);
static mico_mutex_t _mutex;

static OSStatus _FTCRespondInComingMessage(int fd, HTTPHeader_t* inHeader, mico_Context_t * const inContext);
static mico_timer_t _Led_EL_timer;
static bool _FTCClientConnected = false;

static uint8_t *httpResponse = NULL;
static HTTPHeader_t *httpHeader = NULL;

extern OSStatus ConfigIncommingJsonMessage( const char *input, mico_Context_t * const inContext );

extern OSStatus ConfigCreateReportJsonMessage( mico_Context_t * const inContext );

extern void ConfigWillStart( mico_Context_t * const inContext );

extern void ConfigWillStop(mico_Context_t * const inContext );

extern OSStatus ConfigELRecvAuthData(char * userInfo, mico_Context_t * const inContext );

void EasyLinkNotify_WifiStatusHandler(WiFiEvent event, mico_Context_t * const inContext)
{
  easylink_log_trace();
  require(inContext, exit);
  switch (event) {
  case NOTIFY_STATION_UP:
    easylink_log("Access point connected");
    mico_rtos_set_semaphore(&inContext->micoStatus.easylink_sem);
    break;
  case NOTIFY_STATION_DOWN:
    break;
  default:
    break;
  }
exit:
  return;
}

void EasyLinkNotify_EasyLinkButtonClickedHandler(mico_Context_t * const inContext)
{
  (void)inContext;
}


void EasyLinkNotify_WiFIParaChangedHandler(apinfo_adv_t *ap_info, char *key, int key_len, mico_Context_t * const inContext)
{
  easylink_log_trace();
  require(inContext, exit);
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  memcpy(inContext->flashContentInRam.micoSystemConfig.ssid, ap_info->ssid, maxSsidLen);
  memcpy(inContext->flashContentInRam.micoSystemConfig.bssid, ap_info->bssid, 6);
  inContext->flashContentInRam.micoSystemConfig.channel = ap_info->channel;
  inContext->flashContentInRam.micoSystemConfig.security = ap_info->security;
  memcpy(inContext->flashContentInRam.micoSystemConfig.key, key, maxKeyLen);
  inContext->flashContentInRam.micoSystemConfig.keyLength = key_len;
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
exit:
  return;
}

void EasyLinkNotify_DHCPCompleteHandler(net_para_st *pnet, mico_Context_t * const inContext)
{
  easylink_log_trace();
  require(inContext, exit);
  strcpy((char *)inContext->micoStatus.localIp, pnet->ip);
  strcpy((char *)inContext->micoStatus.netMask, pnet->mask);
  strcpy((char *)inContext->micoStatus.gateWay, pnet->gate);
  strcpy((char *)inContext->micoStatus.dnsServer, pnet->dns);
exit:
  return;
}

void EasyLinkNotify_EasyLinkCompleteHandler(network_InitTypeDef_st *nwkpara, mico_Context_t * const inContext)
{
  OSStatus err;
  easylink_log_trace();
  easylink_log("EasyLink return @ %d", mico_get_time());
  require_action(inContext, exit, err = kParamErr);
  require_action(nwkpara, exit, err = kTimeoutErr);

  
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  memcpy(inContext->flashContentInRam.micoSystemConfig.ssid, nwkpara->wifi_ssid, maxSsidLen);
  memcpy(inContext->flashContentInRam.micoSystemConfig.user_key, nwkpara->wifi_key, maxKeyLen);
  inContext->flashContentInRam.micoSystemConfig.user_keyLength = strlen(nwkpara->wifi_key);
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
  easylink_log("Get SSID: %s, Key: %s", inContext->flashContentInRam.micoSystemConfig.ssid, inContext->flashContentInRam.micoSystemConfig.user_key);
  return;

/*EasyLink is not start*/    
exit:
  easylink_log("ERROR, err: %d", err);
  ConfigWillStop(inContext);
  /*so roll back to previous settings  (if it has) and reboot*/
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  if(inContext->flashContentInRam.micoSystemConfig.configured != unConfigured){
    inContext->flashContentInRam.micoSystemConfig.configured = allConfigured;
    MICOUpdateConfiguration(inContext);
    PlatformSoftReboot();
  }
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
  /*module should powd down in default setting*/ 
  wifi_power_down();
  mico_stop_timer(&_Led_EL_timer);
  Platform_LED_SYS_Set_Status(OFF);
  
  return;
}

// Data = AuthData#FTCServer(localIp/netMask/gateWay/dnsServer)
void EasyLinkNotify_EasyLinkGetExtraDataHandler(int datalen, char* data, mico_Context_t * const inContext)
{
  OSStatus err;
  int index ;
  char address[16];
  easylink_log_trace();
  uint32_t *ipInfo, ipInfoCount;
  require_action(inContext, exit, err = kParamErr);

  for(index = datalen; index>=0; index-- ){
    if(data[index] == '#')
      break;
  }
  require_action(index >= 0, exit, err = kParamErr);

  data[index++] = 0x0;
  ipInfo = (uint32_t *)&data[index];
  ipInfoCount = (datalen - index)/sizeof(uint32_t);
  require_action(ipInfoCount >= 1, exit, err = kParamErr);
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  inContext->flashContentInRam.micoSystemConfig.easylinkServerIP = *(uint32_t *)(ipInfo);

  if(ipInfoCount == 1){
    inContext->flashContentInRam.micoSystemConfig.dhcpEnable = true;
    inet_ntoa( address, inContext->flashContentInRam.micoSystemConfig.easylinkServerIP);
    easylink_log("Get auth info: %s, EasyLink server ip address: %s", data, address);
  }else{
    inContext->flashContentInRam.micoSystemConfig.dhcpEnable = false;
    ipInfo = (uint32_t *)&data[index];
    inet_ntoa(inContext->flashContentInRam.micoSystemConfig.localIp, *(uint32_t *)(ipInfo+1));
    inet_ntoa(inContext->flashContentInRam.micoSystemConfig.netMask, *(uint32_t *)(ipInfo+2));
    inet_ntoa(inContext->flashContentInRam.micoSystemConfig.gateWay, *(uint32_t *)(ipInfo+3));
    inet_ntoa(inContext->flashContentInRam.micoSystemConfig.dnsServer, *(uint32_t *)(ipInfo+4));
    strcpy((char *)inContext->micoStatus.localIp, inContext->flashContentInRam.micoSystemConfig.localIp);
    strcpy((char *)inContext->micoStatus.netMask, inContext->flashContentInRam.micoSystemConfig.netMask);
    strcpy((char *)inContext->micoStatus.gateWay, inContext->flashContentInRam.micoSystemConfig.gateWay);
    strcpy((char *)inContext->micoStatus.dnsServer, inContext->flashContentInRam.micoSystemConfig.dnsServer);
    inet_ntoa( address, inContext->flashContentInRam.micoSystemConfig.easylinkServerIP);
    easylink_log("Get auth info: %s, EasyLink server ip address: %s, local IP info:%s %s %s %s ", data, address, inContext->flashContentInRam.micoSystemConfig.localIp,\
    inContext->flashContentInRam.micoSystemConfig.netMask, inContext->flashContentInRam.micoSystemConfig.gateWay,inContext->flashContentInRam.micoSystemConfig.dnsServer);
  }
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);

  require_noerr(ConfigELRecvAuthData(data, inContext), exit);
  
  mico_rtos_set_semaphore(&inContext->micoStatus.easylink_sem);

  return;

exit:
  easylink_log("ERROR, err: %d", err);
  ConfigWillStop(inContext);
  PlatformSoftReboot();
}

void EasyLinkNotify_SYSWillPoerOffHandler(mico_Context_t * const inContext)
{
  stopEasyLink(inContext);
}


static void _led_EL_Timeout_handler( void* arg )
{
  (void)(arg);
  Platform_LED_SYS_Set_Status(TRIGGER);
}

OSStatus startEasyLink( mico_Context_t * const inContext)
{
  easylink_log_trace();

  OSStatus err = kUnknownErr;
  require_action(inContext->micoStatus.easylink_thread_handler==NULL, exit, err = kParamErr);
  require_action(inContext->micoStatus.easylink_sem==NULL, exit, err = kParamErr);
  inContext->micoStatus.easylinkClient_fd = -1;
  //ps_enable();
  mico_mcu_powersave_config(true);
  mico_rtos_init_mutex(&_mutex);

  err = MICOAddNotification( mico_notify_WIFI_STATUS_CHANGED, (void *)EasyLinkNotify_WifiStatusHandler );
  require_noerr(err, exit);
  err = MICOAddNotification( mico_notify_WiFI_PARA_CHANGED, (void *)EasyLinkNotify_WiFIParaChangedHandler );
  require_noerr(err, exit);
  err = MICOAddNotification( mico_notify_EASYLINK_COMPLETED, (void *)EasyLinkNotify_EasyLinkCompleteHandler );
  require_noerr(err, exit);
  err = MICOAddNotification( mico_notify_EASYLINK_GET_EXTRA_DATA, (void *)EasyLinkNotify_EasyLinkGetExtraDataHandler );
  require_noerr(err, exit);
  err = MICOAddNotification( mico_notify_DHCP_COMPLETED, (void *)EasyLinkNotify_DHCPCompleteHandler );
  require_noerr( err, exit );    
  err = MICOAddNotification( mico_notify_SYS_WILL_POWER_OFF, (void *)EasyLinkNotify_SYSWillPoerOffHandler );
  require_noerr( err, exit ); 

  /*Led trigger*/
  mico_init_timer(&_Led_EL_timer, LED_EL_TRIGGER_INTERVAL, _led_EL_Timeout_handler, NULL);
  mico_start_timer(&_Led_EL_timer);

  // Start the EasyLink thread
  ConfigWillStart(inContext);
  mico_rtos_init_semaphore(&inContext->micoStatus.easylink_sem, 1);
  err = mico_rtos_create_thread(&inContext->micoStatus.easylink_thread_handler, MICO_APPLICATION_PRIORITY, "EASYLINK", easylink_thread, 0x500, (void*)inContext );
  require_noerr_action( err, exit, easylink_log("ERROR: Unable to start the EasyLink thread.") );

exit:
  return err;
}

void _easylinkConnectWiFi( mico_Context_t * const inContext)
{
  easylink_log_trace();
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
  StartAdvNetwork(&wNetConfig);
  easylink_log("connect to %s.....\r\n", wNetConfig.ap_info.ssid);
}

void _easylinkConnectWiFi_fast( mico_Context_t * const inContext)
{
  easylink_log_trace();
  network_InitTypeDef_adv_st wNetConfig;
  memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_adv_st));

  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  strncpy((char*)wNetConfig.ap_info.ssid, inContext->flashContentInRam.micoSystemConfig.ssid, maxSsidLen);
  memcpy(wNetConfig.ap_info.bssid, inContext->flashContentInRam.micoSystemConfig.bssid, 6);
  wNetConfig.ap_info.channel = inContext->flashContentInRam.micoSystemConfig.channel;
  wNetConfig.ap_info.security = inContext->flashContentInRam.micoSystemConfig.security;
  memcpy(wNetConfig.key, inContext->flashContentInRam.micoSystemConfig.key, maxKeyLen);
  wNetConfig.key_len = inContext->flashContentInRam.micoSystemConfig.keyLength;
  wNetConfig.dhcpMode = inContext->flashContentInRam.micoSystemConfig.dhcpEnable;
  strncpy((char*)wNetConfig.local_ip_addr, inContext->flashContentInRam.micoSystemConfig.localIp, maxIpLen);
  strncpy((char*)wNetConfig.net_mask, inContext->flashContentInRam.micoSystemConfig.netMask, maxIpLen);
  strncpy((char*)wNetConfig.gateway_ip_addr, inContext->flashContentInRam.micoSystemConfig.gateWay, maxIpLen);
  strncpy((char*)wNetConfig.dnsServer_ip_addr, inContext->flashContentInRam.micoSystemConfig.dnsServer, maxIpLen);

  wNetConfig.wifi_retry_interval = 100;
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
  StartAdvNetwork(&wNetConfig);
  easylink_log("connect to %s.....\r\n", wNetConfig.ap_info.ssid);
}

OSStatus _connectFTCServer( mico_Context_t * const inContext, int *fd)
{
  OSStatus err;
  struct sockaddr_t addr;
  const char *json_str;
  
  size_t httpResponseLen = 0;

  *fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  addr.s_ip = inContext->flashContentInRam.micoSystemConfig.easylinkServerIP; 
  addr.s_port = FTC_PORT;  
  err = connect(*fd, &addr, sizeof(addr));
  require_noerr(err, exit);
  _FTCClientConnected = true;

  easylink_log("Connect to FTC server success, fd: %d", *fd);

  err = ConfigCreateReportJsonMessage( inContext );
  require_noerr( err, exit );

  json_str = json_object_to_json_string(inContext->micoStatus.easylink_report);
  require( json_str, exit );

  easylink_log("Send config object=%s\n", json_str);
  err =  CreateSimpleHTTPMessage_URL( kEasyLinkURLAuth, kMIMEType_JSON, (uint8_t *)json_str, strlen(json_str), &httpResponse, &httpResponseLen );
  require_noerr( err, exit );
  require( httpResponse, exit );

  json_object_put(inContext->micoStatus.easylink_report);

  err = SocketSend( *fd, httpResponse, httpResponseLen );
  free(httpResponse);
  require_noerr( err, exit );
  easylink_log("Current configuration sent");

exit:
  return err;
}


void _cleanEasyLinkResource( mico_Context_t * const inContext )
{
  if(inContext->micoStatus.easylinkClient_fd!=-1)
    close(inContext->micoStatus.easylinkClient_fd);

  /*module should power down under default setting*/ 
  MICORemoveNotification( mico_notify_WIFI_STATUS_CHANGED, (void *)EasyLinkNotify_WifiStatusHandler );
  MICORemoveNotification( mico_notify_WiFI_PARA_CHANGED, (void *)EasyLinkNotify_WiFIParaChangedHandler );
  MICORemoveNotification( mico_notify_EASYLINK_COMPLETED, (void *)EasyLinkNotify_EasyLinkCompleteHandler );
  MICORemoveNotification( mico_notify_EASYLINK_GET_EXTRA_DATA, (void *)EasyLinkNotify_EasyLinkGetExtraDataHandler );

  mico_rtos_deinit_semaphore(&inContext->micoStatus.easylink_sem);
  inContext->micoStatus.easylink_sem = NULL;
  if(httpHeader) free(httpHeader);
  mico_stop_timer(&_Led_EL_timer);
}

OSStatus stopEasyLink( mico_Context_t * const inContext)
{
  (void)inContext;
  if(_FTCClientConnected == true)
    close(inContext->micoStatus.easylinkClient_fd);
  return kNoErr;
}

void easylink_thread(void *inContext)
{
  OSStatus err = kNoErr;
  mico_Context_t *Context = inContext;
  fd_set readfds;
  struct timeval_t t;
  int reConnCount = 0;

  easylink_log_trace();
  require_action(Context->micoStatus.easylink_sem, threadexit, err = kParamErr);

  
  if(Context->flashContentInRam.micoSystemConfig.easyLinkEnable != false){
    OpenEasylink2_withdata(EasyLink_TimeOut); 
    easylink_log("Start easylink @ %d", mico_get_time());
    mico_rtos_get_semaphore(&Context->micoStatus.easylink_sem, MICO_WAIT_FOREVER);
    _easylinkConnectWiFi(Context);
  }else{
    mico_rtos_lock_mutex(&Context->flashContentInRam_mutex);
    Context->flashContentInRam.micoSystemConfig.easyLinkEnable = true;
    MICOUpdateConfiguration(Context);
    mico_rtos_unlock_mutex(&Context->flashContentInRam_mutex);
    _easylinkConnectWiFi_fast(Context);
  }

  err = mico_rtos_get_semaphore(&Context->micoStatus.easylink_sem, ConnectFTC_Timeout);
  require_noerr(err, reboot);

  httpHeader = malloc( sizeof( HTTPHeader_t ) );
  require_action( httpHeader, threadexit, err = kNoMemoryErr );
  HTTPHeaderClear( httpHeader );
  
  t.tv_sec = 100;
  t.tv_usec = 0;
    
  while(1){
    if(Context->micoStatus.easylinkClient_fd == -1){
      err = _connectFTCServer(inContext, &Context->micoStatus.easylinkClient_fd);
      require_noerr(err, Reconn);
    }else{
      FD_ZERO(&readfds);  
      FD_SET(Context->micoStatus.easylinkClient_fd, &readfds);

      err = select(1, &readfds, NULL, NULL, &t);
      require(err > 0, Reconn);
    
      if(FD_ISSET(Context->micoStatus.easylinkClient_fd, &readfds)){
        err = SocketReadHTTPHeader( Context->micoStatus.easylinkClient_fd, httpHeader );

        switch ( err )
        {
          case kNoErr:
            // Read the rest of the HTTP body if necessary
            err = SocketReadHTTPBody( Context->micoStatus.easylinkClient_fd, httpHeader );
            require_noerr(err, Reconn);

            PrintHTTPHeader(httpHeader);
            // Call the HTTPServer owner back with the acquired HTTP header
            err = _FTCRespondInComingMessage( Context->micoStatus.easylinkClient_fd, httpHeader, Context );
            require_noerr( err, Reconn );
            // Reuse HTTPHeader
            HTTPHeaderClear( httpHeader );
          break;

          case EWOULDBLOCK:
              // NO-OP, keep reading
          break;

          case kNoSpaceErr:
            easylink_log("ERROR: Cannot fit HTTPHeader.");
            goto Reconn;
          break;

          case kConnectionErr:
            // NOTE: kConnectionErr from SocketReadHTTPHeader means it's closed
            easylink_log("ERROR: Connection closed.");
            goto threadexit;
             //goto Reconn;
          break;
          default:
            easylink_log("ERROR: HTTP Header parse internal error: %d", err);
            goto Reconn;
        }
      }
    }
    continue;
Reconn:
    HTTPHeaderClear( httpHeader );
    close(Context->micoStatus.easylinkClient_fd);
    Context->micoStatus.easylinkClient_fd = -1;
    require(reConnCount < 6, threadexit);
    reConnCount++;
    sleep(5);
  }  

/*Module is ignored by FTC server, */    
threadexit:
  ConfigWillStop( Context );
  _cleanEasyLinkResource( Context );

  /*Roll back to previous settings (if it has) and reboot*/
  mico_rtos_lock_mutex(&Context->flashContentInRam_mutex);
  if(Context->flashContentInRam.micoSystemConfig.configured != unConfigured){
    Context->flashContentInRam.micoSystemConfig.configured = allConfigured;
    MICOUpdateConfiguration( Context );
    PlatformSoftReboot();
  }
  mico_rtos_unlock_mutex(&Context->flashContentInRam_mutex);


  wifi_power_down();
  Context->micoStatus.easylink_thread_handler = NULL;
  mico_rtos_delete_thread( NULL );
  return;

/*SSID or Password is not correct, module cannot connect to wlan, so reboot and enter EasyLink again*/
reboot:
  ConfigWillStop( Context );
  PlatformSoftReboot();
  return;
}

OSStatus _FTCRespondInComingMessage(int fd, HTTPHeader_t* inHeader, mico_Context_t * const inContext)
{
    OSStatus err = kUnknownErr;
    const char *        value;
    size_t              valueSize;

    easylink_log_trace();

    switch(inHeader->statusCode){
      case kStatusAccept:
        easylink_log("Easylink server accepted!");
        err = kNoErr;
        goto exit;
      break;
      case kStatusOK:
        easylink_log("Easylink server respond status OK!");
        err = HTTPGetHeaderField( inHeader->buf, inHeader->len, "Content-Type", NULL, NULL, &value, &valueSize, NULL );
        require_noerr(err, exit);
        if( strnicmpx( value, valueSize, kMIMEType_JSON ) == 0 ){
          easylink_log("Receive JSON config data!");
          err = ConfigIncommingJsonMessage( inHeader->extraDataPtr, inContext);
          close(fd);
          sleep(2);  //wait for perform TCP close 
          fd = -1;
          PlatformSoftReboot();
        }else if(strnicmpx( value, valueSize, kMIMEType_MXCHIP_OTA ) == 0){
          easylink_log("Receive OTA data!");
          mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
          memset(&inContext->flashContentInRam.bootTable, 0, sizeof(boot_table_t));
          inContext->flashContentInRam.bootTable.length = inHeader->contentLength;
          inContext->flashContentInRam.bootTable.start_address = UPDATE_START_ADDRESS;
          inContext->flashContentInRam.bootTable.type = 'A';
          inContext->flashContentInRam.bootTable.upgrade_type = 'U';
          inContext->flashContentInRam.micoSystemConfig.easyLinkEnable = false;
          MICOUpdateConfiguration(inContext);
          mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
          close(fd);
          sleep(2);  //wait for perform TCP close 
          fd = -1;
          PlatformSoftReboot();
        }else{
          return kUnsupportedDataErr;
        }
        err = kNoErr;
        goto exit;
      break;
      default:
        goto exit;
    }

 exit:
    return err;

}



