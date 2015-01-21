/**
******************************************************************************
* @file    MicoVirtualDevice.h 
* @author  Eshen Wang
* @version V0.2.0
* @date    21-Nov-2014
* @brief   This header contains the defines 
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

#ifndef __MICO_MVD_DEF_H_
#define __MICO_MVD_DEF_H_

#include "EasyCloudServiceDef.h"

/*******************************************************************************
 * DEFINES
 ******************************************************************************/

// default device settings
#define DEFAULT_LOGIN_ID                 "admin"
#define DEFAULT_DEV_PASSWD               "admin"
//#define DEFAULT_USER_TOKEN               "88888888"    // use MAC instead
   
// default device info
#define DEFAULT_DEVICE_ID                "none"
#define DEFAULT_DEVICE_KEY               "none"

#define STACK_SIZE_MVD_MAIN_THREAD       0x500

/* device auto activate mechanism, comment out if not need */
#define DEVICE_AUTO_ACTIVATE_ENABLE      1

/* MQTT topic sub-level */
#define PUBLISH_TOPIC_CHANNEL_STATUS     "status"


/*******************************************************************************
 * STRUCTURES
 ******************************************************************************/

/* device configurations stored in flash */
typedef struct _virtual_device_config_t
{
  /* MCU connect settings */
  uint32_t          USART_BaudRate;
  
  /* cloud connect settings */
  bool              isActivated;                         // device activate status, RO
  char              deviceId[MAX_SIZE_DEVICE_ID];        // get from cloud server, RO
  char              masterDeviceKey[MAX_SIZE_DEVICE_KEY];// get from cloud server, RO
  char              romVersion[MAX_SIZE_FW_VERSION];
  
  char              loginId[MAX_SIZE_LOGIN_ID];
  char              devPasswd[MAX_SIZE_DEV_PASSWD];
  char              userToken[MAX_SIZE_USER_TOKEN];
} virtual_device_config_t;

/* device status */
typedef struct _virtual_device_status_t
{
  bool              isCloudConnected;   // cloud service connect
  uint64_t          RecvRomFileSize;    // return OTA data size for bootTable.length
} virtual_device_status_t;

typedef struct _virtual_device_context_t {
  virtual_device_config_t config_info;  // virtual device config info
  virtual_device_status_t status;       //virtual device running status
} virtual_device_context_t;


#endif
