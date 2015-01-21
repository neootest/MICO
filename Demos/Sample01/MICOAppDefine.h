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

#define APP_INFO   "mxchipWNet Sample Demo based on MICO OS"

#define FIRMWARE_REVISION   "MICO_SMP_0_1"
#define MANUFACTURER        "MXCHIP Inc."
#define SERIAL_NUMBER       "2015Jan16"
#define PROTOCOL            "com.mxchip.spp"

/* Wi-Fi configuration mode */
//#define MICO_CONFIG_MODE CONFIG_MODE_EASYLINK_WITH_SOFTAP
/*User provided configurations*/
#define CONFIGURATION_VERSION               0x00000001 // if default configuration is changed, update this number

/* Define thread stack size */
#ifdef DEBUG
  #define STACK_SIZE_UART_RECV_THREAD           0x2A0
  #define STACK_SIZE_LOCAL_TCP_SERVER_THREAD    0x300
  #define STACK_SIZE_LOCAL_TCP_CLIENT_THREAD    0x350
  #define STACK_SIZE_REMOTE_TCP_CLIENT_THREAD   0x500
#else
  #define STACK_SIZE_UART_RECV_THREAD           0x150
  #define STACK_SIZE_LOCAL_TCP_SERVER_THREAD    0x180
  #define STACK_SIZE_LOCAL_TCP_CLIENT_THREAD    0x200
  #define STACK_SIZE_REMOTE_TCP_CLIENT_THREAD   0x260
#endif

/*Application's configuration stores in flash*/
typedef struct{
  uint32_t          configDataVer;
    uint32_t sample_led;
} application_config_t;

typedef struct _current_app_status_t {
    uint32_t led_on;
} current_app_status_t;


#endif

