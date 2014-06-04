#ifndef __MICO_CALLBACK_H__
#define __MICO_CALLBACK_H__

#include "mico_define.h"

// void ApListCallback(UwtPara_str *pApList);

typedef enum {
  MXCHIP_WIFI_UP = 1,
  MXCHIP_WIFI_DOWN,

  MXCHIP_UAP_UP,
  MXCHIP_UAP_DOWN,
} WiFiEvent;

typedef enum{
  /* MICO system defined notifications */
  mico_notify_WIFI_SCAN_COMPLETED,          //void (*function)(ScanResult *pApList, mico_Context_t * const inContext);
  mico_notify_WIFI_STATUS_CHANGED,          //void (*function)(WiFiEvent status, mico_Context_t * const inContext);
  mico_notify_WiFI_PARA_CHANGED,            //void (*function)(apinfo_adv_t *ap_info, char *key, int key_len, mico_Context_t * const inContext);
  mico_notify_DHCP_COMPLETED,               //void (*function)(net_para_st *pnet, mico_Context_t * const inContext);
  mico_notify_EASYLINK_COMPLETED,           //void (*function)(network_InitTypeDef_st *nwkpara, mico_Context_t * const inContext);
  mico_notify_EASYLINK_GET_EXTRA_DATA,      //void (*function)(int datalen, char*data, mico_Context_t * const inContext);
  mico_notify_TCP_CLIENT_CONNECTED,         //void (*function)(char *str, int len, mico_Context_t * const inContext);
  mico_notify_DNS_RESOLVE_COMPLETED,        //void (*function)(char *str, int len, mico_Context_t * const inContext);
  mico_notify_READ_APP_INFO,                //void (*function)(int fd, mico_Context_t * const inContext);
  mico_notify_SYS_WILL_POWER_OFF,            //void (*function)(mico_Context_t * const inContext);
  
  /* User defined notifications */

} mico_notify_types_t;

OSStatus mico_init_notification_center  ( void * const inContext );

OSStatus mico_add_notification          ( mico_notify_types_t notify_type, void *functionAddress );

OSStatus mico_remove_notification       ( mico_notify_types_t notify_type, void *functionAddress );


#endif


