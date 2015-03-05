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
   
// default device info
#define DEFAULT_DEVICE_ID                "none"
#define DEFAULT_DEVICE_KEY               "none"

#define STACK_SIZE_MVD_MAIN_THREAD       0x500

/* MQTT topic sub-level, device_id/out/status */
#define PUBLISH_TOPIC_CHANNEL_STATUS     "status"


/*******************************************************************************
 * STRUCTURES
 ******************************************************************************/

/* device configurations stored in flash */
typedef struct _virtual_device_config_t
{
  /* MCU connect settings */
  uint32_t          USART_BaudRate;
  
  /* cloud connect params */
  bool              isActivated;                         // device activate flag
  char              deviceId[MAX_SIZE_DEVICE_ID];        // get from cloud server
  char              masterDeviceKey[MAX_SIZE_DEVICE_KEY];// get from cloud server
  char              romVersion[MAX_SIZE_FW_VERSION];     // get from cloud server
  
  char              loginId[MAX_SIZE_LOGIN_ID];          // not used for wechat dev
  char              devPasswd[MAX_SIZE_DEV_PASSWD];      // not used for wechat dev
  //char              userToken[MAX_SIZE_USER_TOKEN];    // use MAC addr instead
  
  /* reset flag */
  bool              needCloudReset;                     // need reset cloud when set
} virtual_device_config_t;

/* device status */
typedef struct _virtual_device_status_t
{
  bool              isCloudConnected;   // cloud service connect status
  uint64_t          RecvRomFileSize;    // return OTA data size for bootTable.length, 0 means not need to update
} virtual_device_status_t;

typedef struct _virtual_device_context_t {
  virtual_device_config_t config_info;  // virtual device config info
  virtual_device_status_t status;       // virtual device running status
} virtual_device_context_t;

#endif
