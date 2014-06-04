/**
  ******************************************************************************
  * @file    mico_main.c 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   MICO system main entrance.
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

#include "platform.h"
#include "mico_api.h"
#include "EasyLink.h"
#include "StringUtils.h"
#include "mico_define.h"
#include "mico_app_define.h"


static mico_Context_t *context;
static mico_timer_t _watchdog_reload_timer;

static mico_system_monitor_t mico_monitor;

#define mico_log(M, ...) custom_log("MICO", M, ##__VA_ARGS__)
#define mico_log_trace() custom_log_trace("MICO")

__weak void sendNotifySYSWillPowerOff(void){

}

/* ========================================
User provide callback functions 
======================================== */
void micoNotify_ReadAppInfoHandler(char *str, int len, mico_Context_t * const inContext)
{
  (void)inContext;
  snprintf( str, len, "%s", APP_INFO);
}   


void PlatformEasyLinkButtonClickedCallback(void)
{
  mico_log_trace();
  if(context->flashContentInRam.micoSystemConfig.configured == allConfigured){
    context->flashContentInRam.micoSystemConfig.configured = wLanUnConfigured;
    mico_UpdateConfiguration(context);
  }
  context->micoStatus.sys_state = eState_Software_Reset;
  require(context->micoStatus.sys_state_change_sem, exit);
  mico_rtos_set_semaphore(&context->micoStatus.sys_state_change_sem);
exit: 
  return;
}

void PlatformEasyLinkButtonLongPressedCallback(void)
{
  mico_log_trace();
  mico_RestoreDefault(context);
  context->micoStatus.sys_state = eState_Software_Reset;
  require(context->micoStatus.sys_state_change_sem, exit);
  mico_rtos_set_semaphore(&context->micoStatus.sys_state_change_sem);
exit: 
  return;
}

 void PlatformStandbyButtonClickedCallback(void)
 {
    mico_log_trace();
    context->micoStatus.sys_state = eState_Standby;
    require(context->micoStatus.sys_state_change_sem, exit);
    mico_rtos_set_semaphore(&context->micoStatus.sys_state_change_sem);
exit: 
    return;
 }

void micoNotify_WifiStatusHandler(WiFiEvent event, mico_Context_t * const inContext)
{
  mico_log_trace();
  (void)inContext;
  switch (event) {
  case MXCHIP_WIFI_UP:
    mico_log("Station up");
    break;
  case MXCHIP_WIFI_DOWN:
    mico_log("Station down");
    break;
  default:
    break;
  }
  return;
}

void micoNotify_DHCPCompleteHandler(net_para_st *pnet, mico_Context_t * const inContext)
{
  mico_log_trace();
  require(inContext, exit);
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  strcpy((char *)inContext->micoStatus.localIp, pnet->ip);
  strcpy((char *)inContext->micoStatus.netMask, pnet->mask);
  strcpy((char *)inContext->micoStatus.gateWay, pnet->gate);
  strcpy((char *)inContext->micoStatus.dnsServer, pnet->dns);
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
exit:
  return;
}

void micoNotify_WiFIParaChangedHandler(apinfo_adv_t *ap_info, char *key, int key_len, mico_Context_t * const inContext)
{
  mico_log_trace();
  bool _needsUpdate = false;
  require(inContext, exit);
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  if(strncmp(inContext->flashContentInRam.micoSystemConfig.ssid, ap_info->ssid, maxSsidLen)!=0){
    strncpy(inContext->flashContentInRam.micoSystemConfig.ssid, ap_info->ssid, maxSsidLen);
    _needsUpdate = true;
  }

  if(memcmp(inContext->flashContentInRam.micoSystemConfig.bssid, ap_info->bssid, 6)!=0){
    memcpy(inContext->flashContentInRam.micoSystemConfig.bssid, ap_info->bssid, 6);
    _needsUpdate = true;
  }

  if(inContext->flashContentInRam.micoSystemConfig.channel != ap_info->channel){
    inContext->flashContentInRam.micoSystemConfig.channel = ap_info->channel;
    _needsUpdate = true;
  }
  
  if(inContext->flashContentInRam.micoSystemConfig.security != ap_info->security){
    inContext->flashContentInRam.micoSystemConfig.security = ap_info->security;
    _needsUpdate = true;
  }

  if(memcmp(inContext->flashContentInRam.micoSystemConfig.key, key, maxKeyLen)!=0){
    memcpy(inContext->flashContentInRam.micoSystemConfig.key, key, maxKeyLen);
    _needsUpdate = true;
  }

  if(inContext->flashContentInRam.micoSystemConfig.keyLength != key_len){
    inContext->flashContentInRam.micoSystemConfig.keyLength = key_len;
    _needsUpdate = true;
  }

  if(_needsUpdate== true)  
    mico_UpdateConfiguration(inContext);
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
  
exit:
  return;
}

void micoMainConnectConnect( mico_Context_t * const inContext)
{
  mico_log_trace();
  network_InitTypeDef_adv_st wNetConfig;
  mico_log("connect to %s.....", inContext->flashContentInRam.micoSystemConfig.ssid);
  memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_adv_st));
  
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  strncpy((char*)wNetConfig.ap_info.ssid, inContext->flashContentInRam.micoSystemConfig.ssid, maxSsidLen);
  memcpy(wNetConfig.ap_info.bssid, inContext->flashContentInRam.micoSystemConfig.bssid, 6);
  wNetConfig.ap_info.channel = inContext->flashContentInRam.micoSystemConfig.channel;
  wNetConfig.ap_info.security = inContext->flashContentInRam.micoSystemConfig.security;
  memcpy(wNetConfig.key, inContext->flashContentInRam.micoSystemConfig.key, inContext->flashContentInRam.micoSystemConfig.keyLength);
  wNetConfig.key_len = inContext->flashContentInRam.micoSystemConfig.keyLength;
  wNetConfig.dhcpMode = inContext->flashContentInRam.micoSystemConfig.dhcpEnable;
  strncpy((char*)wNetConfig.local_ip_addr, inContext->flashContentInRam.micoSystemConfig.localIp, maxIpLen);
  strncpy((char*)wNetConfig.net_mask, inContext->flashContentInRam.micoSystemConfig.netMask, maxIpLen);
  strncpy((char*)wNetConfig.gateway_ip_addr, inContext->flashContentInRam.micoSystemConfig.gateWay, maxIpLen);
  strncpy((char*)wNetConfig.dnsServer_ip_addr, inContext->flashContentInRam.micoSystemConfig.dnsServer, maxIpLen);
  
  if(wNetConfig.dhcpMode != DHCP_Client){
    strcpy((char *)inContext->micoStatus.localIp, inContext->flashContentInRam.micoSystemConfig.localIp);
    strcpy((char *)inContext->micoStatus.netMask, inContext->flashContentInRam.micoSystemConfig.netMask);
    strcpy((char *)inContext->micoStatus.gateWay, inContext->flashContentInRam.micoSystemConfig.gateWay);
    strcpy((char *)inContext->micoStatus.dnsServer, inContext->flashContentInRam.micoSystemConfig.dnsServer);
  }
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);

  wNetConfig.wifi_retry_interval = 100;
  StartAdvNetwork(&wNetConfig);
}

static void _watchdog_reload_timer_handler( void* arg )
{
  (void)(arg);
  mico_update_system_monitor(&mico_monitor, APPLICATION_WATCHDOG_TIMEOUT_SECONDS*1000-100);
}

int application_start(void)
{
  OSStatus err = kNoErr;
  net_para_st para;

  /*STM32 wakeup by watchdog in standby mode, re-enter standby mode in this situation*/
  PlatformWDGReload();
  if ( (PWR_GetFlagStatus(PWR_FLAG_SB) != RESET) && RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
  {
    RCC_ClearFlag();
    Platform_Enter_STANDBY();
  }
  PWR_ClearFlag(PWR_FLAG_SB);

  Platform_Init();
  /*Read current configurations*/
  context = ( mico_Context_t *)malloc(sizeof(mico_Context_t) );
  require_action( context, exit, err = kNoMemoryErr );
  memset(context, 0x0, sizeof(mico_Context_t));
  mico_rtos_init_mutex(&context->flashContentInRam_mutex);
  mico_rtos_init_semaphore(&context->micoStatus.sys_state_change_sem, 1); 

  mico_ReadConfiguration( context );

  err = mico_init_notification_center  ( context );

  err = mico_add_notification( mico_notify_READ_APP_INFO, (void *)micoNotify_ReadAppInfoHandler );
  require_noerr( err, exit );  
  
  /*wlan driver and tcpip init*/
  mxchipInit();
  getNetPara(&para, Station);
  formatMACAddr(context->micoStatus.mac, (char *)&para.mac);
  
  mico_log_trace(); 
  mico_log("%s mxchipWNet library version: %s", APP_INFO, system_lib_version());

  /*Start system monotor thread*/
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "SYS MONITOR", mico_system_monitor_thread_main, 0x500, (void*)context );
  require_noerr_action( err, exit, mico_log("ERROR: Unable to start the system monitor thread.") );
  
  err = mico_register_system_monitor(&mico_monitor, APPLICATION_WATCHDOG_TIMEOUT_SECONDS*1000);
  require_noerr( err, exit );
  mico_init_timer(&_watchdog_reload_timer,APPLICATION_WATCHDOG_TIMEOUT_SECONDS*1000-100, _watchdog_reload_timer_handler, NULL);
  mico_start_timer(&_watchdog_reload_timer);

  
  if(context->flashContentInRam.micoSystemConfig.configured != allConfigured){
    mico_log("Empty configuration. Starting EasyLink configuration...");
    err = startEasyLink( context );
    require_noerr( err, exit );
  }
  else{
    mico_log("Available configuration. Starting Wi-Fi connection...");
    
    /* Regisist notifications */
    err = mico_add_notification( mico_notify_WIFI_STATUS_CHANGED, (void *)micoNotify_WifiStatusHandler );
    require_noerr( err, exit ); 
    
    err = mico_add_notification( mico_notify_WiFI_PARA_CHANGED, (void *)micoNotify_WiFIParaChangedHandler );
    require_noerr( err, exit ); 

    err = mico_add_notification( mico_notify_DHCP_COMPLETED, (void *)micoNotify_DHCPCompleteHandler );
    require_noerr( err, exit );  
   
    if(context->flashContentInRam.micoSystemConfig.rfPowerSaveEnable == true){
      ps_enable();
    }

    if(context->flashContentInRam.micoSystemConfig.mcuPowerSaveEnable == true){
      mico_mcu_powersave_config(true);
    }

    /*Bonjour service for searching*/
    if(context->flashContentInRam.micoSystemConfig.bonjourEnable == true){
      err = startBonjourService( context );
      require_noerr( err, exit );
    }

    /*Local configuration server*/
    err =  startConfigurationServer(context);
    require_noerr_action( err, exit, mico_log("ERROR: Unable to start the local server thread.") );

    /*Start mico application*/
    err = startMicoApplication( context );
    require_noerr( err, exit );
    
    micoMainConnectConnect( context );
  }


  /*System status changed*/
  while(mico_rtos_get_semaphore(&context->micoStatus.sys_state_change_sem, MICO_WAIT_FOREVER)==kNoErr){
    switch(context->micoStatus.sys_state){
      case eState_Normal:
        break;
      case eState_Software_Reset:
        sendNotifySYSWillPowerOff();
        msleep(500);
        PlatformSoftReboot();
        break;
      case eState_Wlan_Powerdown:
        sendNotifySYSWillPowerOff();
        msleep(500);
        wifi_power_down();
        break;
      case eState_Standby:
        mico_log("Enter standby mode");
        sendNotifySYSWillPowerOff();
        msleep(100);
        wifi_power_down();
        Platform_Enter_STANDBY();
        break;
      default:
        break;
    }
  }
    
  require_noerr_action( err, exit, mico_log("Closing main thread with err num: %d.", err) );

exit:
  mico_rtos_delete_thread(NULL);
  return kNoErr;
}


