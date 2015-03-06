/**
  ******************************************************************************
  * @file    MICOAppDefine.h 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   This file create a TCP listener thread, accept every TCP client
  *          connection and create thread for them.
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


#ifndef __MICOAPPDEFINE_H
#define __MICOAPPDEFINE_H

#include "Common.h"
#include "MicoVirtualDeviceDef.h"


#define APP_INFO   "mxchipWNet Wechat Demo based on MICO OS"

#define FIRMWARE_REVISION   "MICO_WECHAT_LED_1_0"
#define MANUFACTURER        "MXCHIP Inc."
#define SERIAL_NUMBER       "20150305"
#define PROTOCOL            "com.wechat.led"


/* Wi-Fi configuration mode */
//#define MICO_CONFIG_MODE CONFIG_MODE_EASYLINK_PLUS
#define MICO_CONFIG_MODE CONFIG_MODE_AIRKISS

/*User provided configurations*/
#define CONFIGURATION_VERSION               0x00000003 // if default configuration is changed, update this number
#define LOCAL_PORT                          8080
#define BONJOUR_SERVICE                     "_easylink._tcp.local."

/* product type */
#define DEFAULT_PRODUCT_ID        "f2889664"
#define DEFAULT_PRODUCT_KEY       "fc7773f7-9199-4999-8bac-803ec949c61f"
#define DEFAULT_ROM_VERSION       "v0.0.1"

/*Application's configuration stores in flash*/
typedef struct
{
  uint32_t          configDataVer;           // config param update number
  uint32_t          localServerPort;         // for bonjour service port

  virtual_device_config_t virtualDevConfig;  //virtual device settings
} application_config_t;

/*Running status*/
typedef struct _current_app_status_t {
  virtual_device_status_t virtualDevStatus;  //virtual device status
} current_app_status_t;


#endif

