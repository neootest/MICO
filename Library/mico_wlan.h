/**
  ******************************************************************************
  * @file    mico_wlan.h 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   This file provides all the headers of wlan connectivity functions.
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

#ifndef __MICO_WLAN_H__
#define __MICO_WLAN_H__

#include "stdlib.h"
#include "string.h"
#include <stdint.h>
#include "Common.h"

typedef enum { 
  Soft_AP,  /**< Act as an access point, and other station can connect, 4 stations Max*/
  Station   /**< Act as a station which can connect to an access point*/
} WiFi_Interface; 

typedef enum {
  DHCP_Disable,  /**< Disable DHCP service*/
  DHCP_Client,   /**< Enable DHCP client, get IP address from DHCP server*/
  DHCP_Server    /**< Enable DHCP server, address pool is assigned aotomatically
                 depends on MICO's own address*/
} DHCPOperationMode; 


typedef struct _net_para {
  char dhcp;
  char ip[16]; // such as string  "192.168.1.1"
  char gate[16];
  char mask[16];
  char dns[16];
  char mac[16]; // such as string "7E0000001111"
  char broadcastip[16];
} net_para_st;

typedef  enum {
   SECURITY_TYPE_NONE,
   SECURITY_TYPE_WEP,
   SECURITY_TYPE_WPA_TKIP,
   SECURITY_TYPE_WPA_AES,
   SECURITY_TYPE_WPA2_TKIP,
   SECURITY_TYPE_WPA2_AES,
   SECURITY_TYPE_WPA2_MIXED,
   SECURITY_TYPE_AUTO,
} SECURITY_TYPE_E;

typedef  struct  _adv_ap_info 
{ 
    char ssid[32]; 
    char bssid[6];
    char channel;
    SECURITY_TYPE_E security;
}apinfo_adv_t;

typedef  struct  _ApList_str  
{  
  char ssid[32];  
  char ApPower;  // min:0, max:100
}ApList_str; 
 
typedef  struct  _ApList_adv 
{ 
  char ssid[32]; 
  char ApPower;  // min:0, max:100
  char bssid[6];
  char channel;
  SECURITY_TYPE_E security;
}ApList_adv_t;
 
typedef  struct  _ScanResult_adv 
{ 
  char ApNum;       //AP Quantity
  ApList_adv_t * ApList;
} ScanResult_adv; 

typedef  struct  _ScanResult 
{  
  char ApNum;       //AP Quantity
  ApList_str * ApList; 
} ScanResult;  

typedef struct _network_InitTypeDef_st 
{ 
  char wifi_mode;    // SoftAp(0)£¬sta(1)  
  char wifi_ssid[32]; 
  char wifi_key[32]; 
  char local_ip_addr[16]; 
  char net_mask[16]; 
  char gateway_ip_addr[16]; 
  char dnsServer_ip_addr[16]; 
  char dhcpMode;       // disable(0), client mode(1), server mode(2) 
  char address_pool_start[16]; 
  char address_pool_end[16]; 
  int wifi_retry_interval;//sta reconnect interval, ms
} network_InitTypeDef_st; 

typedef struct _network_InitTypeDef_adv_st
{
    apinfo_adv_t ap_info;
    char key[64];
    int key_len;
    char local_ip_addr[16];
    char net_mask[16];
    char gateway_ip_addr[16];
    char dnsServer_ip_addr[16];
    char dhcpMode;       // disable(0), client mode(1), server mode(2)
    char address_pool_start[16];
    char address_pool_end[16];
    int wifi_retry_interval;//sta reconnect interval, ms
} network_InitTypeDef_adv_st;

typedef struct _sta_ap_state{
  int is_connected;
  int wifi_strength;
  uint8_t  ssid[32];
  uint8_t  bssid[6];
}sta_ap_state_t;

struct wifi_InitTypeDef
{
  uint8_t wifi_mode;		// adhoc mode(1), AP client mode(0), AP mode(2)
  uint8_t wifi_ssid[32];
  uint8_t wifi_key[32];
};


/**
  * @brief  Connect or establish a Wi-Fi network in normal mode.
  * @detail This function can establish a Wi-Fi connection as a station or create
  *         a soft AP that other staions can connect. SSID and password are provided
  *         When start a connection in station mode, MICO first scan all the 
  *         supported Wi-Fi channels for the input SSID, and read the security
  *         mode. And try to connect once that target SSID is found. If any 
  *         error occurs in the connection procedure or disconnected after a 
  *         successful connection, MICO restart the procedure in backgound after
  *         a time interval defined pNetworkInitPara.
  * @param  inNetworkInitPara: Specifies wlan parameters.
  * @retval kNoErr.
  */
OSStatus StartNetwork(network_InitTypeDef_st* inNetworkInitPara);

/**
  * @brief  Connect to a Wi-Fi network with advantage settings
  * @detail This function can connect to an access point with precise settings,
  *         that greatly speed up the connection speed if the input settings are
  *         correct. If this fast connection is failed for some reason, MICO 
  *         change back to normal mode(like using StartNetwork()).
  * @note   This function cannot establish a soft ap, use StartNetwork() for this
  *         purpose.
  * @param  inNetworkInitParaAdv: Specifies precise wlan parameters .
  * @retval kNoErr.
  */
OSStatus StartAdvNetwork(network_InitTypeDef_adv_st* inNetworkInitParaAdv);

/**
  * @brief  Read current IP status on a interface.
  * @param  ioNetpara: Point to the buffer to store the IP address. 
  * @param  inInterface: Specifies wlan interface. 
  *             @arg Soft_AP: The soft AP that established by StartNetwork()
  *             @arg Station: The interface that connected to an access point
  * @retval kNoErr.
  */
OSStatus getNetPara(net_para_st * ioNetpara, WiFi_Interface inInterface);

/**
  * @brief  Read current wireless link status on station interface.
  * @param  ioState: Point to the buffer to store the link status. 
  * @retval kNoErr.
  */
OSStatus CheckNetLink(sta_ap_state_t *ioState);

/**
  * @brief  Start a wlan scanning in 2.4GHz in MICO backfround
  * @detail Once the scan is completed, MICO sends a callback: 
  *         mico_notify_WIFI_SCAN_COMPLETED,
  *         with function:
  *         void (*function)(ScanResult *pApList, mico_Context_t * const inContext)
  *         So,you need to register this call back using mico_add_notification().
  */
void mxchipStartScan(void);

/**
  * @brief  Close the RF's power supply
  * @retval kNoErr.
  */
OSStatus wifi_power_down(void);

/**
  * @brief  Open the RF's power supply and do some necessary initialization
  * @retval kNoErr.
  */
OSStatus wifi_power_up(void);

/**
  * @brief  Close all the wireless connection
  * @note   This function also stop the background retry mechanism started by 
            StartNetwork() and StartAdvNetwork()
  * @retval kNoErr.
  */
OSStatus wlan_disconnect(void);

/**
  * @brief  Close the connection in station mode
  * @note   This function also stop the background retry mechanism started by 
            StartNetwork() and StartAdvNetwork()
  * @retval kNoErr.
  */
OSStatus sta_disconnect(void);

/**
  * @brief  Close the connection in soft ap mode
  * @retval kNoErr.
  */
OSStatus uap_stop(void);

/**
  * @brief  Start EasyLink configuration V2
  * @detail MICO can read SSID and password from iOS or android devices where an
  *         Easylink APP is running on. Once easylink is success or timeout, 
  *         MICO sends a callback: mico_notify_EASYLINK_COMPLETED
  *         with function:
  *         void (*function)(network_InitTypeDef_st *nwkpara, mico_Context_t * const inContext).
  *         If nwkpara is NULL, means Easylink is failed or timeout, otherwise,
  *         read SSID and password from nwkpara.
  *         You need to register this callback function using mico_add_notification().
  * @param  inTimeout: If esalink is excuted longer than this parameter in backgound.
  *         MICO stops EasyLink and sends a callback where nwkpara is NULL
  * @retval kNoErr.
  */
OSStatus OpenEasylink2(int inTimeout);

/**
  * @brief  Start EasyLink configuration V2 with user extra data
  * @detail This function has the same function as OpenEasylink2(), but it can 
  *         read more data besides SSID and password, these data is sent from
  *         Easylink APP.  
  *         MICO sends a callback: mico_notify_EASYLINK_GET_EXTRA_DATA
  *         with function:
  *         void (*function)(int datalen, char*data, mico_Context_t * const inContext);
  *         that provide a buffer where the data is saved
  * @param  inTimeout: If esalink is excuted longer than this parameter in backgound.
  *         MICO stops EasyLink and sends a callback where nwkpara is NULL
  * @retval kNoErr.
  */
OSStatus OpenEasylink2_withdata(int inTimeout);

/**
  * @brief  Stop EasyLink configuration procedure
  * @retval kNoErr.
  */
OSStatus CloseEasylink2(void);

/**
  * @brief  Enable IEEE power save mode
  * @detail When this function is enabled, MICO enter IEEE power save mode if
  *         MICO is in station mode and has connected to an AP, and do not need 
  *         any control from application.
  */
void ps_enable(void);

/**
  * @brief  Disable IEEE power save mode
  */
void ps_disable(void); 



#endif //__MICO_WLAN_H__




