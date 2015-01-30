/**
******************************************************************************
* @file    MICOEntrance.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   MICO system main entrance.
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

#define TEST


#include "time.h"
#include "MicoPlatform.h"
#include "platform.h"
#include "platform_common_config.h"
#include "MICODefine.h"
#include "MICOAppDefine.h"
#if 0
#include "MICONotificationCenter.h"
#include "MICOSystemMonitor.h"
#include "EasyLink/EasyLink.h"
#include "SoftAP/EasyLinkSoftAP.h"
#include "WPS/WPS.h"
#include "WAC/MFi_WAC.h"
#endif 
#include "StringUtils.h"

#ifdef TEST
// #define TEST_CONTEXT
// #define TEST_NOTIFICATION   
// #define TEST_EASYLINK       
// #define TEST_APP            
#define TEST_PLATFORM      
#define TEST_WLAN
// #define TEST_TIMER
// #define TEST_TOAP
// #define TEST_CONFIGSERVER
// #define TEST_NTP
// #define TEST_SYSMONITOR
// #define TEST_SYSSTATUS
#endif 

#if defined (CONFIG_MODE_EASYLINK) || defined (CONFIG_MODE_EASYLINK_WITH_SOFTAP)
// #include "EasyLink/EasyLink.h"
#endif
#if defined ( TEST_CONTEXT )
static mico_Context_t *context;
#endif 
#if defined ( TEST_TIMER)
static mico_timer_t _watchdog_reload_timer;

#endif
#if defined ( TEST_SYSMONITOR )
static mico_system_monitor_t mico_monitor;

#endif
#if defined ( TEST_EA )
const char *eaProtocols[1] = {EA_PROTOCOL};
#endif

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
  snprintf( str, len, "%s, build at %s %s", APP_INFO, __TIME__, __DATE__);
}



void PlatformEasyLinkButtonClickedCallback(void)
{
  mico_log_trace();
  bool needsUpdate = false;
  
#if !defined ( TEST ) || defined ( TEST_CONTEXT )
  if(context->flashContentInRam.micoSystemConfig.easyLinkByPass != EASYLINK_BYPASS_NO){
    context->flashContentInRam.micoSystemConfig.easyLinkByPass = EASYLINK_BYPASS_NO;
    needsUpdate = true;
  }

  if(context->flashContentInRam.micoSystemConfig.configured == allConfigured){
    context->flashContentInRam.micoSystemConfig.configured = wLanUnConfigured;
    needsUpdate = true;
  }
  
  if(needsUpdate == true)
    MICOUpdateConfiguration(context);
  
  context->micoStatus.sys_state = eState_Software_Reset;
  require(context->micoStatus.sys_state_change_sem, exit);
  mico_rtos_set_semaphore(&context->micoStatus.sys_state_change_sem);
#endif 
exit: 
  return;
}

void PlatformEasyLinkButtonLongPressedCallback(void)
{
  mico_log_trace();
#if !defined ( TEST ) || defined (TEST_CONTEXT)
  MICORestoreDefault(context);
  context->micoStatus.sys_state = eState_Software_Reset;
  require(context->micoStatus.sys_state_change_sem, exit);
  mico_rtos_set_semaphore(&context->micoStatus.sys_state_change_sem);
#endif 
exit: 
  return;
}

 void PlatformStandbyButtonClickedCallback(void)
 {
    mico_log_trace();
#if !defined ( TEST ) || defined (TEST_CONTEXT)
    context->micoStatus.sys_state = eState_Standby;
    require(context->micoStatus.sys_state_change_sem, exit);
    mico_rtos_set_semaphore(&context->micoStatus.sys_state_change_sem);
#endif 
exit: 
    return;
 }
#if !defined ( TEST ) || defined ( TEST_NOTIFICATION )
void micoNotify_WifiStatusHandler(WiFiEvent event, mico_Context_t * const inContext)
{
  mico_log_trace();
  (void)inContext;
  switch (event) {
  case NOTIFY_STATION_UP:
    mico_log("Station up");
    MicoRfLed(true);
    break;
  case NOTIFY_STATION_DOWN:
    mico_log("Station down");
    MicoRfLed(false);
    break;
  case NOTIFY_AP_UP:
    mico_log("uAP established");
    MicoRfLed(true);
    break;
  case NOTIFY_AP_DOWN:
    mico_log("uAP deleted");
    MicoRfLed(false);
    break;
  default:
    break;
  }
  return;
}
#endif 
void micoNotify_DHCPCompleteHandler(IPStatusTypedef *pnet, mico_Context_t * const inContext)
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
#if !defined ( TEST ) || defined (TEST_CONTEXT)
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
    MICOUpdateConfiguration(inContext);
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
#endif  
exit:
  return;
}


void micoNotify_ConnectFailedHandler(OSStatus err, mico_Context_t * const inContext)
{
  mico_log_trace();
  (void)inContext;
  mico_log("Wlan Connection Err %d", err);
}

void micoNotify_WlanFatalErrHandler(mico_Context_t * const inContext)
{
  mico_log_trace();
  (void)inContext;
  mico_log("Wlan Fatal Err!");
  MicoSystemReboot();
}

void micoNotify_StackOverflowErrHandler(char *taskname, mico_Context_t * const inContext)
{
  mico_log_trace();
  (void)inContext;
  mico_log("Thread %s overflow, system rebooting", taskname);
  MicoSystemReboot(); 
}

void _ConnectToAP( mico_Context_t * const inContext)
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
  if(inContext->flashContentInRam.micoSystemConfig.dhcpEnable == true)
    wNetConfig.dhcpMode = DHCP_Client;
  else
    wNetConfig.dhcpMode = DHCP_Disable;
  strncpy((char*)wNetConfig.local_ip_addr, inContext->flashContentInRam.micoSystemConfig.localIp, maxIpLen);
  strncpy((char*)wNetConfig.net_mask, inContext->flashContentInRam.micoSystemConfig.netMask, maxIpLen);
  strncpy((char*)wNetConfig.gateway_ip_addr, inContext->flashContentInRam.micoSystemConfig.gateWay, maxIpLen);
  strncpy((char*)wNetConfig.dnsServer_ip_addr, inContext->flashContentInRam.micoSystemConfig.dnsServer, maxIpLen);

  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);

  wNetConfig.wifi_retry_interval = 100;
  micoWlanStartAdv(&wNetConfig);
}

static void _watchdog_reload_timer_handler( void* arg )
{
  (void)(arg);
#if !defined ( TEST ) || defined (TEST_SYSMONITOR)
  MICOUpdateSystemMonitor(&mico_monitor, APPLICATION_WATCHDOG_TIMEOUT_SECONDS*1000-100);
#endif 
}

static void mico_mfg_test(void)
{
  int ret;
  extern int mfg_test(char *);
  
  ret = mfg_test("MXCHIP_CAGE");
  if (ret == 0)
    printf("MFG test success\r\n");
  else {
    if (ret & 1) 
      printf("SCAN FAIL\r\n");
    if (ret & 2)
      printf("Connect AP FAIL\r\n");
  }
  
  mico_thread_sleep(MICO_NEVER_TIMEOUT);
}
#if 0
int application_start(void)
{
  OSStatus err = kNoErr;
  IPStatusTypedef para;
  struct tm currentTime;
  mico_rtc_time_t time;
  char wifi_ver[64];
#if !defined ( TEST ) || defined (TEST_CONTEXT)
  /*Read current configurations*/
  context = ( mico_Context_t *)malloc(sizeof(mico_Context_t) );
  require_action( context, exit, err = kNoMemoryErr );
  memset(context, 0x0, sizeof(mico_Context_t));
  mico_rtos_init_mutex(&context->flashContentInRam_mutex);
  mico_rtos_init_semaphore(&context->micoStatus.sys_state_change_sem, 1); 
d
  MICOReadConfiguration( context );
#endif
#if !defined ( TEST ) || defined (TEST_NOTIFICATION)
  err = MICOInitNotificationCenter  ( context );

  err = MICOAddNotification( mico_notify_READ_APP_INFO, (void *)micoNotify_ReadAppInfoHandler );
  require_noerr( err, exit );  

  err = MICOAddNotification( mico_notify_WIFI_CONNECT_FAILED, (void *)micoNotify_ConnectFailedHandler );
  require_noerr( err, exit ); 

  err = MICOAddNotification( mico_notify_WIFI_Fatal_ERROR, (void *)micoNotify_WlanFatalErrHandler );
  require_noerr( err, exit ); 

  err = MICOAddNotification( mico_notify_Stack_Overflow_ERROR, (void *)micoNotify_StackOverflowErrHandler );
  require_noerr( err, exit ); 
#endif 
#if !defined ( TEST ) || defined ( TEST_PLATFORM )
  /*wlan driver and tcpip init*/
  MicoInit();
  MicoSysLed(true);
  mico_log("Free memory %d bytes", MicoGetMemoryInfo()->free_memory) ; 

#endif 
  /* Enter test mode, call a build-in test function amd output on STDIO */
  if(MicoShouldEnterMFGMode()==true)
    mico_mfg_test();

  /*Read current time from RTC.*/
  MicoRtcGetTime(&time);
  currentTime.tm_sec = time.sec;
  currentTime.tm_min = time.min;
  currentTime.tm_hour = time.hr;
  currentTime.tm_mday = time.date;
  currentTime.tm_wday = time.weekday;
  currentTime.tm_mon = time.month - 1;
  currentTime.tm_year = time.year + 100;
  mico_log("Current Time: %s",asctime(&currentTime));
#if !defined ( TEST ) || defined ( TEST_WLAN )
  micoWlanGetIPStatus(&para, Station);
  formatMACAddr(context->micoStatus.mac, (char *)&para.mac);
  wlan_driver_version(wifi_ver, sizeof(wifi_ver));
  mico_log_trace(); 
  mico_log("%s ver: %s, mac %s", APP_INFO, MicoGetVer(), context->micoStatus.mac);
  mico_log("wifi version %s", wifi_ver);
#endif
#if !defined ( TEST ) || defined ( TEST_SYSMONITOR )
  /*Start system monotor thread*/
  err = MICOStartSystemMonitor(context);
  require_noerr_action( err, exit, mico_log("ERROR: Unable to start the system monitor.") );

  err = MICORegisterSystemMonitor(&mico_monitor, APPLICATION_WATCHDOG_TIMEOUT_SECONDS*1000);
  require_noerr( err, exit );
#endif 
#if !defined ( TEST ) || defined ( TEST_TIMER )
  mico_init_timer(&_watchdog_reload_timer,APPLICATION_WATCHDOG_TIMEOUT_SECONDS*1000 - 100, _watchdog_reload_timer_handler, NULL);
  mico_start_timer(&_watchdog_reload_timer);

#endif 
#if !defined ( TEST ) || defined ( TEST_NOTIFICATION )
  /* Regisist notifications */
  err = MICOAddNotification( mico_notify_WIFI_STATUS_CHANGED, (void *)micoNotify_WifiStatusHandler );
  require_noerr( err, exit ); 
#endif 
#ifdef BOARD_LPCXPRESSO_54102
  wifimgr_debug_enable(true);
#endif
#if !defined ( TEST ) || defined (TEST_CONTEXT)
  if(context->flashContentInRam.micoSystemConfig.configured != allConfigured){
    mico_log("Empty configuration. Starting configuration mode...");
#endif 
#if !defined ( TEST ) || defined ( TEST_EASYLINK )
#if (MICO_CONFIG_MODE == CONFIG_MODE_EASYLINK) || (MICO_CONFIG_MODE == CONFIG_MODE_EASYLINK_WITH_SOFTAP)
  err = startEasyLink( context );
  require_noerr( err, exit );
#elif (MICO_CONFIG_MODE == CONFIG_MODE_SOFT_AP)
  err = startEasyLinkSoftAP( context );
  require_noerr( err, exit );
#elif (MICO_CONFIG_MODE == CONFIG_MODE_AIRKISS)
  err = startAirkiss( context );
  require_noerr( err, exit );
#elif (MICO_CONFIG_MODE == CONFIG_MODE_WPS) || MICO_CONFIG_MODE == defined (CONFIG_MODE_WPS_WITH_SOFTAP)
  err = startWPS( context );
  require_noerr( err, exit );
#elif ( MICO_CONFIG_MODE == CONFIG_MODE_WAC)
  WACPlatformParameters_t* WAC_Params = NULL;
  WAC_Params = calloc(1, sizeof(WACPlatformParameters_t));
  require(WAC_Params, exit);

  str2hex((unsigned char *)para.mac, WAC_Params->macAddress, 6);
  WAC_Params->isUnconfigured          = 1;
  WAC_Params->supportsAirPlay         = 0;
  WAC_Params->supportsAirPrint        = 0;
  WAC_Params->supports2_4GHzWiFi      = 1;
  WAC_Params->supports5GHzWiFi        = 0;
  WAC_Params->supportsWakeOnWireless  = 0;

  WAC_Params->firmwareRevision =  FIRMWARE_REVISION;
  WAC_Params->hardwareRevision =  HARDWARE_REVISION;
  WAC_Params->serialNumber =      SERIAL_NUMBER;
  WAC_Params->name =              context->flashContentInRam.micoSystemConfig.name;
  WAC_Params->model =             MODEL;
  WAC_Params->manufacturer =      MANUFACTURER;

  WAC_Params->numEAProtocols =    1;
  WAC_Params->eaBundleSeedID =    BUNDLE_SEED_ID;
  WAC_Params->eaProtocols =       (char **)eaProtocols;

  err = startMFiWAC( context, WAC_Params, 1200);
  free(WAC_Params);
  require_noerr( err, exit );
#else
  #error "Wi-Fi configuration mode is not defined"?
#endif
#endif 
#if !defined ( TEST ) || defined (TEST_CONTEXT)
  }
  else{
#endif
    mico_log("Available configuration. Starting Wi-Fi connection...");
    
#if !defined ( TEST ) || defined ( TEST_NOTIFICATION )
    err = MICOAddNotification( mico_notify_WiFI_PARA_CHANGED, (void *)micoNotify_WiFIParaChangedHandler );
    require_noerr( err, exit ); 

    err = MICOAddNotification( mico_notify_DHCP_COMPLETED, (void *)micoNotify_DHCPCompleteHandler );
    require_noerr( err, exit );  
#endif   
#if !defined ( TEST ) || defined (TEST_CONTEXT)
    if(context->flashContentInRam.micoSystemConfig.rfPowerSaveEnable == true){
      micoWlanEnablePowerSave();
    }

    if(context->flashContentInRam.micoSystemConfig.mcuPowerSaveEnable == true){
      MicoMcuPowerSaveConfig(true);
    }
#endif
#if !defined ( TEST ) || defined ( TEST_CONFIGSERVER )
    /*Local configuration server*/
    if(context->flashContentInRam.micoSystemConfig.configServerEnable == true){
      err =  MICOStartConfigServer(context);
      require_noerr_action( err, exit, mico_log("ERROR: Unable to start the local server thread.") );
    }
#endif 
#if !defined ( TEST ) || defined ( TEST_NTP )
    err =  MICOStartNTPClient(context);
    require_noerr_action( err, exit, mico_log("ERROR: Unable to start the NTP client thread.") );

#endif 
#if !defined ( TEST ) || defined ( TEST_APP )
    /*Start mico application*/
    err = MICOStartApplication( context );
    require_noerr( err, exit );

#endif 
#if !defined ( TEST ) || defined ( TEST_TOAP )
    _ConnectToAP( context );
#endif 
#if !defined ( TEST ) || defined (TEST_CONTEXT)
  }
#endif
  mico_log("Free memory %d bytes", MicoGetMemoryInfo()->free_memory) ; 
  
#if !defined ( TEST ) || defined ( TEST_SYSSTATUS )
  /*System status changed*/
  while(mico_rtos_get_semaphore(&context->micoStatus.sys_state_change_sem, MICO_WAIT_FOREVER)==kNoErr){
    switch(context->micoStatus.sys_state){
      case eState_Normal:
        break;
      case eState_Software_Reset:
        sendNotifySYSWillPowerOff();
        mico_thread_msleep(500);
        MicoSystemReboot();
        break;
      case eState_Wlan_Powerdown:
        sendNotifySYSWillPowerOff();
        mico_thread_msleep(500);
        micoWlanPowerOff();
        break;
      case eState_Standby:
        mico_log("Enter standby mode");
        sendNotifySYSWillPowerOff();
        mico_thread_msleep(200);
        micoWlanPowerOff();
        MicoSystemStandBy(MICO_WAIT_FOREVER);
        break;
      default:
        break;
    }
  }
    
#endif 
  require_noerr_action( err, exit, mico_log("Closing main thread with err num: %d.", err) );

exit:
  mico_rtos_delete_thread(NULL);
  return kNoErr;
}
#else 
int application_start(void)
{
  OSStatus err = kNoErr;
#if !defined ( TEST ) || defined ( TEST_WLAN )
  IPStatusTypedef para;
  char wifi_ver[64];
#endif
  //struct tm currentTime;
  //mico_rtc_time_t time;
#if defined(__CC_ARM)
	mico_log("Build by Keil");
#elif defined (__IAR_SYSTEMS_ICC__)
	mico_log("Build by IAR");
#endif
#if !defined ( TEST ) || defined ( TEST_PLATFORM )
  /*wlan driver and tcpip init*/
  MicoInit();
  MicoSysLed(true);
  mico_log("Free memory %d bytes", MicoGetMemoryInfo()->free_memory) ; 

#endif 
#if !defined ( TEST ) || defined ( TEST_WLAN )
  micoWlanGetIPStatus(&para, Station);
  #if !defined ( TEST ) || defined ( TEST_CONTEXT )
  formatMACAddr(context->micoStatus.mac, (char *)&para.mac);
  #endif 
  wlan_driver_version(wifi_ver, sizeof(wifi_ver));
  mico_log_trace(); 
  #if !defined ( TEST ) || defined ( TEST_CONTEXT )
  mico_log("%s ver: %s, mac %s", APP_INFO, MicoGetVer(), context->micoStatus.mac);
  #else
  mico_log("%s ver: %s, ", APP_INFO, MicoGetVer() );
  #endif 
  mico_log("wifi version %s", wifi_ver);
#endif
  return kNoErr;
}
#endif 

