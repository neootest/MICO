/**
  ******************************************************************************
  * @file    MICOAppDefine.h 
  * @version V1.0.0
  *
  * 
*/


#ifndef __MICOAPPDEFINE_H
#define __MICOAPPDEFINE_H

#include "Common.h"

#define APP_INFO   "mxchip PRGM Demo based on MICO OS"

#define FIRMWARE_REVISION   "MICO_PRGM_FLSH"
#define MANUFACTURER        "MXCHIP Inc."
#define SERIAL_NUMBER       "20150106"

#define MICO_CONFIG_MODE CONFIG_MODE_EASYLINK_PLUS

#define CONFIGURATION_VERSION               0x00000001 // if default configuration is changed, update this number
#define MAX_Local_Client_Num               1 

/*Application's configuration stores in flash*/
typedef struct
{
  uint32_t          configDataVer;
  uint32_t          localServerPort;

  /*local services*/
  bool              localServerEnable;
  bool              remoteServerEnable;
  char              remoteServerDomain[64];
  int               remoteServerPort;

  /*IO settings*/
  uint32_t          USART_BaudRate;
} application_config_t;

/*Running status*/
typedef struct _current_app_status_t {
  /*Local clients port list*/
  uint32_t          loopBack_PortList[MAX_Local_Client_Num];
  /*Remote TCP client connecte*/
  bool              isRemoteConnected;
} current_app_status_t;

void Program_thread(void *arg);


#endif

