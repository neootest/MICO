//
//  MFi_WAC.c
//
//  Created by William Xu on 14-2-14.
//  Copyright (c) 2014  MXCHIP. All rights reserved.

#include "stdio.h"
#include <string.h>
//#include "MFi_WAC/MFi_WAC.h"
#include "MICODefine.h"
#include "MICONotificationCenter.h"
#include "MICO.h"
#include "Platform.h"
#include "MDNSUtils.h"
#include "MFi-SAP.h"
//#include "MFi_WAC/debug.h"
#include "PlatformMFiAuth.h"
//#include "MFi_WAC/platform/PlatformRandomNumber.h"
#include   "WACTLV.h"
#include   "StringUtils.h"
#include "WACLogging.h"

#define Support_AirPlay_Offset         (7)
#define Device_unconfigured_Offset     (6)
#define Support_MFi_ConfigV1_Offset    (5)
#define Support_WoW_Offset             (4)
#define Robustness_enabled_Offset      (3)
#define PPPoE_Server_Offset            (2)
#define Support_WPS_Offset             (1)
#define WPS_Active_Offset              (0)

#define Support_AirPrint_Offset        (3)
#define Reserved_Offset                (2)
#define Support_2_4_GHz_Offset         (1)
#define Support_5_GHz_Offset           (0)

/*Device info under apple MAC configuration*/
const char *WAC_NAME_default = "MXCHIP Wi-Fi Module";
const char *WAC_Manufacturer_default = "MXCHIP Inc.";

#ifdef EMW3162
const char *WAC_Model_default = "EMW3162";
#define MFi_INSTANCE_NAME                  "MXCHIP_3162_MFi."
#define MFi_HOST_NAME                      "EMW3162_MFi.local."
#endif

#ifdef EMW3161
const char *WAC_Model_default = "EMW3161";
#define MFi_INSTANCE_NAME                  "MXCHIP_3161_MFi."
#define MFi_HOST_NAME                      "EMW3161_MFi.local."
#endif

#define MFi_SERVICE_MFi                    "_mfi-config._tcp.local."
#define MFi_SERVICE_PORT                   65520


const char *BundleSeedID = BUNDLE_SEED_ID;  //AYDR2YFZ4K
const char *eaProtocols[1] = {EA_PROTOCOL};

const uint8_t WAC_OUI_default[3] = {0x00, 0xB7, 0x43};
const uint8_t WAC_dWDS_default[2] = {0x00, 0x00};
const uint8_t WAC_BlueTooth_MAC_default[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static mico_thread_t mfi_tcp_server_thread_handler;

FunctionalState _suspend_MFi_bonjour = DISABLE;
static WACPlatformParameters_t* WAC_Params = NULL;
static bonjour_init_t bonjour_init;

mico_semaphore_t wac_success = NULL;


#define APP_Available_Offset               0
#define Support_TLV_Config_Offset          2

#define Problem_Detected_Offset            0
#define Not_Configured_Offset              1

static uint32_t _gBonjourSeedID = 1;
static bonjour_init_t init;
static mico_timer_t _Led_EL_timer;
static void _mfi_bonjour_init(uint8_t app_available, WiFi_Interface interface, mico_Context_t * const inContext);

//static int mDNS_fd = -1;
//static int wac_bonjour_announce = 0;

/** Enumeration of WICED interfaces. \n
 * @note The config interface is a virtual interface that shares the softAP interface
 */
typedef enum
{
    WICED_STA_INTERFACE     = 0, /**< STA or Client Interface  */
    WICED_AP_INTERFACE      = 1, /**< softAP Interface         */
    WICED_CONFIG_INTERFACE  = 3, /**< config softAP Interface  */
} wiced_interface_t;

/**
 * Enumeration of custom IE management actions
 */
typedef enum
{
    WICED_ADD_CUSTOM_IE,     /**< Add a custom IE    */
    WICED_REMOVE_CUSTOM_IE   /**< Remove a custom IE */
} wiced_custom_ie_action_t;

/**
 * Enumeration of applicable packet mask bits for custom Information Elements (IEs)
 */
typedef enum
{
    VENDOR_IE_BEACON         = 0x1,  /**< Denotes beacon packet                  */
    VENDOR_IE_PROBE_RESPONSE = 0x2,  /**< Denotes probe response packet          */
    VENDOR_IE_ASSOC_RESPONSE = 0x4,  /**< Denotes association response packet    */
    VENDOR_IE_AUTH_RESPONSE  = 0x8,  /**< Denotes authentication response packet */
    VENDOR_IE_PROBE_REQUEST  = 0x10, /**< Denotes probe request packet           */
    VENDOR_IE_ASSOC_REQUEST  = 0x20, /**< Denotes association request packet     */
    VENDOR_IE_CUSTOM         = 0x100 /**< Denotes a custom IE identifier         */
} wiced_ie_packet_flag_t;

/**
 * Enumeration of Apple Device IE elements
 */
typedef enum
{
    Element_ID_Flags            = 0x0,  
    Element_ID_Name             = 0x1,  
    Element_ID_Manufacturer     = 0x2,  
    Element_ID_Model            = 0x3,  
    Element_ID_OUI              = 0x4, 
    Element_ID_dWDS             = 0x5, 
    Element_ID_Bluetooth_MAC    = 0x6,
    Element_ID_Device_ID        = 0x7
} Element_ID_t;

/**
 * Apple Inc. OUI reserved for this IE
 */
const uint8_t oui[3] = {0x00, 0xA0, 0x40};
const uint8_t sub_type = 0x0;
uint8_t App_Available = 0;

static int WacTimeOut = 0;

/** Manage the addition and removal of custom IEs
 *
 * @param action       : the action to take (add or remove IE)
 * @param out          : the oui of the custom IE
 * @param subtype      : the IE sub-type
 * @param data         : a pointer to the buffer that hold the custom IE
 * @param length       : the length of the buffer pointed to by 'data'
 * @param which_packets: a mask of which packets this IE should be included in. See wiced_ie_packet_flag_t
 *
 * @return WICED_SUCCESS : if the custom IE action was successful
 *         WICED_ERROR   : if the custom IE action failed
 */
extern OSStatus wiced_wifi_manage_custom_ie( wiced_interface_t interface, wiced_custom_ie_action_t action, /*@unique@*/ uint8_t* oui, uint8_t subtype, void* data, uint16_t length, uint16_t which_packets );

void udp_multicast_thread(void *arg);
void led_manage_thread(void *arg);

static void _led_EL_Timeout_handler( void* arg )
{
  (void)(arg);
  Platform_LED_SYS_Set_Status(TRIGGER);
}

void WACNotify_WiFIParaChangedHandler(apinfo_adv_t *ap_info, char *key, int key_len, mico_Context_t * const inContext)
{
  wac_log_trace();
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

void WACNotify_WifiStatusHandler(WiFiEvent event, mico_Context_t * const inContext)
{
  wac_log_trace();
  (void)inContext;
  switch (event) {
  case NOTIFY_STATION_UP:
    wac_log("Station Connected");
    break;
  default:
    break;
  }
  return;
}

void WACNotify_DHCPCompleteHandler(net_para_st *pnet, mico_Context_t * const inContext)
{
  wac_log_trace();
  (void)inContext;
  (void)pnet;
  wac_log("DHCP Success");
  suspend_bonjour_service(DISABLE);
}

OSStatus startMfiWac( mico_Context_t * const inContext)
{
  int i, ret=0;
  uint8_t flag1=0, flag2=0;
  uint8_t *Elements = NULL;
  uint8_t *ptemp = NULL;
  uint16_t ElementsLength = 0;
  network_InitTypeDef_st WAC_NetConfig;
  net_para_st para;
  OSStatus err = kNoErr;

  mico_rtos_init_semaphore(&wac_success, 1);

  require(WAC_Params==NULL, exit);
  WAC_Params = calloc(1, sizeof(WACPlatformParameters_t));
  require(WAC_Params, exit);

  err = MICOAddNotification( mico_notify_WiFI_PARA_CHANGED, (void *)WACNotify_WiFIParaChangedHandler );
  require_noerr(err, exit);
  err = MICOAddNotification( mico_notify_WIFI_STATUS_CHANGED, (void *)WACNotify_WifiStatusHandler );
  require_noerr(err, exit);
  err = MICOAddNotification( mico_notify_DHCP_COMPLETED, (void *)WACNotify_DHCPCompleteHandler );
  require_noerr( err, exit );   

  str2hex((unsigned char *)para.mac, WAC_Params->macAddress, 6);
  WAC_Params->isUnconfigured          = 1;
  WAC_Params->supportsAirPlay         = 0;
  WAC_Params->supportsAirPrint        = 0;
  WAC_Params->supports2_4GHzWiFi      = 1;
  WAC_Params->supports5GHzWiFi        = 0;
  WAC_Params->supportsWakeOnWireless  = 0;

  WAC_Params->firmwareRevision =  inContext->flashContentInRam.micoSystemConfig.firmwareRevision;
  WAC_Params->hardwareRevision =  inContext->flashContentInRam.micoSystemConfig.hardwareRevision;
  WAC_Params->serialNumber =      inContext->flashContentInRam.micoSystemConfig.SerialNumber;
  WAC_Params->name =              inContext->flashContentInRam.micoSystemConfig.name;
  WAC_Params->model =             inContext->flashContentInRam.micoSystemConfig.model;
  WAC_Params->manufacturer =      inContext->flashContentInRam.micoSystemConfig.manufacturer;

  WAC_Params->numEAProtocols =    1;
  WAC_Params->eaBundleSeedID =    (char *)BundleSeedID;
  WAC_Params->eaProtocols =       (char **)eaProtocols;

  flag1 = WAC_Params->supportsAirPlay<<Support_AirPlay_Offset|
          WAC_Params->isUnconfigured<<Device_unconfigured_Offset|
          1<<Support_MFi_ConfigV1_Offset|
          WAC_Params->supportsWakeOnWireless<<Support_WoW_Offset|
          0<<Robustness_enabled_Offset|
          0<<PPPoE_Server_Offset|
          1<<Support_WPS_Offset|
          0<<WPS_Active_Offset;

  flag2 = WAC_Params->supportsAirPrint<<Support_AirPrint_Offset|
          0<<Reserved_Offset|
          WAC_Params->supports2_4GHzWiFi<<Support_2_4_GHz_Offset|
          WAC_Params->supports5GHzWiFi<<Support_5_GHz_Offset;

  ElementsLength = 34+strlen(WAC_Params->name)+strlen(WAC_Params->manufacturer)+strlen(WAC_Params->model);

  if(flag2 != 0)
    ElementsLength ++;

  if((Elements = malloc(ElementsLength))==NULL)
    return -1;
  ptemp = Elements;

  for(i=0; i<8; i++){
    *ptemp = i;

    switch(i){
      case Element_ID_Flags:
        if(flag2 != 0)
          *(++ptemp) = 2;
        else
          *(++ptemp) = 1;

        *(++ptemp) = flag1;
        
        if(flag2 != 0)
          *(++ptemp) = flag2;

        ptemp++;
        break;
      case Element_ID_Name:
        *(++ptemp) = strlen(WAC_Params->name);
        memcpy(++ptemp, WAC_Params->name, strlen(WAC_Params->name));
        ptemp += strlen(WAC_Params->name);
        break;
      case Element_ID_Manufacturer:
        *(++ptemp) = strlen(WAC_Params->manufacturer);
        memcpy(++ptemp, WAC_Params->manufacturer, strlen(WAC_Params->manufacturer));
        ptemp += strlen(WAC_Params->manufacturer);
        break;
      case Element_ID_Model:
        *(++ptemp) = strlen(WAC_Params->model);
        memcpy(++ptemp, WAC_Params->model, strlen(WAC_Params->model));
        ptemp += strlen(WAC_Params->model);
        break;
      case Element_ID_OUI:
        *(++ptemp) = 3;
        memcpy(++ptemp, oui, 3);
        ptemp += 3;
        break;
      case Element_ID_dWDS:
        *(++ptemp) = 2;
        memcpy(++ptemp, WAC_dWDS_default, 2);
        ptemp += 2;
        break;
      case Element_ID_Bluetooth_MAC:
        *(++ptemp) = 6;
        memcpy(++ptemp, WAC_BlueTooth_MAC_default, 6);
        ptemp += 6;
        break;
      case Element_ID_Device_ID:
        *(++ptemp) = 6;
        getNetPara(&para, Soft_AP);
        str2hex((unsigned char *)para.mac, ++ptemp, 6);
        ptemp += 6;
        break;
    }
  }

  memset(&WAC_NetConfig, 0, sizeof(struct _network_InitTypeDef_st));
  WAC_NetConfig.wifi_mode = Soft_AP;
  sprintf(WAC_NetConfig.wifi_ssid, "MXCHIP_%s", &para.mac[6]);
  strcpy((char*)WAC_NetConfig.wifi_key, "");
  strcpy((char*)WAC_NetConfig.local_ip_addr, "10.10.10.1");
  strcpy((char*)WAC_NetConfig.net_mask, "255.255.255.0");
  strcpy((char*)WAC_NetConfig.gateway_ip_addr, "10.10.10.1");
  strcpy((char*)WAC_NetConfig.address_pool_start, "10.10.10.10");
  strcpy((char*)WAC_NetConfig.address_pool_end, "10.10.10.177");
  WAC_NetConfig.dhcpMode = DHCP_Server;
  StartNetwork(&WAC_NetConfig);
  
  err = wiced_wifi_manage_custom_ie( WICED_AP_INTERFACE, 
                                      WICED_ADD_CUSTOM_IE, 
                                      (uint8_t *)oui, 
                                      sub_type, 
                                      (void *)Elements, 
                                      ElementsLength, 
                                      VENDOR_IE_BEACON|VENDOR_IE_PROBE_RESPONSE);
  require_noerr(err, exit);


  ret =  PlatformMFiAuthInitialize();
  require_noerr(err, exit);

  /*Led trigger*/
  mico_init_timer(&_Led_EL_timer, LED_WAC_TRIGGER_INTERVAL, _led_EL_Timeout_handler, NULL);
  mico_start_timer(&_Led_EL_timer);

  App_Available = (WAC_Params->numEAProtocols)? 1:0;

  _mfi_bonjour_init(App_Available, Soft_AP, inContext);
  start_bonjour_service();

  ret = mico_rtos_create_thread(&mfi_tcp_server_thread_handler, MICO_APPLICATION_PRIORITY, "MFI_TCP_SERVER", http_server_thread, 0x2500, (void*)inContext );
  
exit:  
if(Elements) free(Elements);
  return ret;
}

int CloseMfiWac(void)
{
  return 0;
}

OSStatus applyNewConfiguration(char* destinationSSID, char *destinationPSK, char *accessoryName, char *playPassword, mico_Context_t * const inContext)
{
  wac_log_trace();
  OSStatus err = kParamErr;
  network_InitTypeDef_adv_st WAC_NetConfig;

  require(destinationSSID, exit);

  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  memset(&WAC_NetConfig, 0, sizeof(network_InitTypeDef_adv_st));
  strncpy((char*)WAC_NetConfig.ap_info.ssid, destinationSSID, maxSsidLen);
  WAC_NetConfig.ap_info.security = SECURITY_TYPE_AUTO;
  memcpy(WAC_NetConfig.key, destinationPSK, maxKeyLen);
  WAC_NetConfig.key_len = strlen(destinationPSK);
  WAC_NetConfig.dhcpMode = DHCP_Client;
  WAC_NetConfig.wifi_retry_interval = 100;

  memcpy(inContext->flashContentInRam.micoSystemConfig.name, accessoryName, maxNameLen);
  inContext->flashContentInRam.micoSystemConfig.name[maxNameLen-1] = 0x0;
  inContext->flashContentInRam.micoSystemConfig.dhcpEnable = true;

  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
  
  (void)playPassword;
  
  uap_stop();
  msleep(200);
  StartAdvNetwork(&WAC_NetConfig);
  inContext->flashContentInRam.micoSystemConfig.seed ++;
  _mfi_bonjour_init(App_Available, Station, inContext);
  return kNoErr;

exit:
  return err;  
}



OSStatus CreateTLVConfigResponseMessage( uint8_t **outTLVResponse, size_t *outTLVResponseLen)
{
    wac_log_trace();
    OSStatus err = kParamErr;

    uint8_t *eaProtocolSizes        = NULL;
    uint8_t nameSize                = 0;
    uint8_t manufacturerSize        = 0;
    uint8_t modelSize               = 0;
    uint8_t firmwareRevisionSize    = 0;
    uint8_t hardwareRevisionSize    = 0;
    uint8_t serialNumberSize        = 0;
    uint8_t eaBundleSeedIDSize      = 0;

    *outTLVResponse = NULL;
    *outTLVResponseLen = 0;

    require( WAC_Params, exit );

    if ( WAC_Params->name )
    {
        nameSize = strnlen( WAC_Params->name, kWACTLV_MaxStringSize );
        *outTLVResponseLen += nameSize + kWACTLV_TypeLengthSize;
    }

    if ( WAC_Params->manufacturer )
    {
        manufacturerSize = strnlen( WAC_Params->manufacturer, kWACTLV_MaxStringSize );
        *outTLVResponseLen += manufacturerSize + kWACTLV_TypeLengthSize;
    }

    if ( WAC_Params->model )
    {
        modelSize = strnlen( WAC_Params->model, kWACTLV_MaxStringSize );
        *outTLVResponseLen += modelSize + kWACTLV_TypeLengthSize;
    }

    if ( WAC_Params->firmwareRevision )
    {
        firmwareRevisionSize = strnlen( WAC_Params->firmwareRevision, kWACTLV_MaxStringSize );
        *outTLVResponseLen += firmwareRevisionSize + kWACTLV_TypeLengthSize;
    }

    if ( WAC_Params->hardwareRevision )
    {
        hardwareRevisionSize = strnlen( WAC_Params->hardwareRevision, kWACTLV_MaxStringSize );
        *outTLVResponseLen += hardwareRevisionSize + kWACTLV_TypeLengthSize;
    }

    if ( WAC_Params->serialNumber )
    {
        serialNumberSize = strnlen( WAC_Params->serialNumber, kWACTLV_MaxStringSize );
        *outTLVResponseLen += serialNumberSize + kWACTLV_TypeLengthSize;
    }

    if ( WAC_Params->eaBundleSeedID )
    {
        eaBundleSeedIDSize = strnlen( WAC_Params->eaBundleSeedID, kWACTLV_MaxStringSize );
        *outTLVResponseLen += eaBundleSeedIDSize + kWACTLV_TypeLengthSize;
    }

    eaProtocolSizes = calloc( WAC_Params->numEAProtocols, sizeof( uint32_t ) );
    require_action( eaProtocolSizes, exit, err = kNoMemoryErr );

    uint8_t protocolIndex;
    for( protocolIndex = 0; protocolIndex < WAC_Params->numEAProtocols; protocolIndex++ )
    {
        eaProtocolSizes[protocolIndex] = strnlen( WAC_Params->eaProtocols[protocolIndex], kWACTLV_MaxStringSize );
        *outTLVResponseLen += eaProtocolSizes[protocolIndex] + kWACTLV_TypeLengthSize;
    }

    // Allocate space for the entire TLV
    *outTLVResponse = calloc( *outTLVResponseLen, sizeof( uint8_t ) );
    require_action( *outTLVResponse, exit, err = kNoMemoryErr );

    uint8_t *tlvPtr = *outTLVResponse;

    // Accessory Name
    if ( nameSize )
    {
        *tlvPtr++ = kWACTLV_Name;
        *tlvPtr++ = nameSize;
        memcpy( tlvPtr, WAC_Params->name, nameSize );
        tlvPtr += nameSize;
    }

    // Accessory Manufacturer
    if ( manufacturerSize )
    {
        *tlvPtr++ = kWACTLV_Manufacturer;
        *tlvPtr++ = manufacturerSize;
        memcpy( tlvPtr, WAC_Params->manufacturer, manufacturerSize );
        tlvPtr += manufacturerSize;
    }

    // Accessory Model
    if ( modelSize )
    {
        *tlvPtr++ = kWACTLV_Model;
        *tlvPtr++ = modelSize;
        memcpy( tlvPtr, WAC_Params->model, modelSize );
        tlvPtr += modelSize;
    }

    // Serial Number
    if ( serialNumberSize )
    {
        *tlvPtr++ = kWACTLV_SerialNumber;
        *tlvPtr++ = serialNumberSize;
        memcpy( tlvPtr, WAC_Params->serialNumber, serialNumberSize );
        tlvPtr += serialNumberSize;
    }

    // Firmware Revision
    if ( firmwareRevisionSize )
    {
        *tlvPtr++ = kWACTLV_FirmwareRevision;
        *tlvPtr++ = firmwareRevisionSize;
        memcpy( tlvPtr, WAC_Params->firmwareRevision, firmwareRevisionSize );
        tlvPtr += firmwareRevisionSize;
    }

    // Hardware Revision
    if ( hardwareRevisionSize )
    {
        *tlvPtr++ = kWACTLV_HardwareRevision;
        *tlvPtr++ = hardwareRevisionSize;
        memcpy( tlvPtr, WAC_Params->hardwareRevision, hardwareRevisionSize );
        tlvPtr += hardwareRevisionSize;
    }

    // EA Protocols
    for( protocolIndex = 0; protocolIndex < WAC_Params->numEAProtocols; protocolIndex++ )
    {
        *tlvPtr++ = kWACTLV_MFiProtocol;
        *tlvPtr++ = eaProtocolSizes[protocolIndex];
        memcpy( tlvPtr, WAC_Params->eaProtocols[protocolIndex], eaProtocolSizes[protocolIndex] );
        tlvPtr += eaProtocolSizes[protocolIndex];
    }

    // BundleSeedID
    if ( eaBundleSeedIDSize )
    {
        *tlvPtr++ = kWACTLV_BundleSeedID;
        *tlvPtr++ = eaBundleSeedIDSize;
        memcpy( tlvPtr, WAC_Params->eaBundleSeedID, eaBundleSeedIDSize );
        tlvPtr += eaBundleSeedIDSize;
    }

    require_action( ( tlvPtr - *outTLVResponseLen ) == *outTLVResponse, exit, err = kSizeErr );

    err = kNoErr;

exit:
    if( err && *outTLVResponse ) free( *outTLVResponse );
    if( eaProtocolSizes )        free( eaProtocolSizes );

    return err;
}

static void _mfi_bonjour_init(uint8_t app_available, WiFi_Interface interface, mico_Context_t * const inContext)
{
  int len;
  char temp_txt[100];
  bonjour_init_t init;
  char *temp_txt2;

  memset(&init, 0x0, sizeof(bonjour_init_t));

  init.interface = interface;

  init.service_name = MFi_SERVICE_MFi;
  init.host_name = MFi_HOST_NAME;
  init.instance_name = (char*)__strdup(inContext->flashContentInRam.micoSystemConfig.name);
  init.service_port = 65520;

  wac_log("Seed number: %d.", inContext->flashContentInRam.micoSystemConfig.seed);

  temp_txt2 = __strdup_trans_dot(inContext->micoStatus.mac);
  sprintf(temp_txt, "deviceid=%s.", temp_txt2);
  free(temp_txt2);

  sprintf(temp_txt, "%sfeatures=%x.", temp_txt, 1<<Support_TLV_Config_Offset + 0);

  sprintf(temp_txt, "%sseed=%d.", temp_txt, inContext->flashContentInRam.micoSystemConfig.seed);

  temp_txt2 = __strdup_trans_dot("1.0");
  sprintf(temp_txt, "%ssrcvers=%s.", temp_txt, temp_txt2);
  free(temp_txt2);

  init.txt_record = temp_txt;
  bonjour_service_init(init);
}




