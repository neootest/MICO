/**
  ******************************************************************************
  * @file    EasyLink.h 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   This file provide header file for the easylink function and FTC server 
  *            for quick provisioning and first time configuration.
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


#ifndef __EASYLINK_H
#define __EASYLINK_H

#include "mico_define.h"

#define FTC_PORT 8000
#define WiFiConnect_TIMEOUT  200000 //20s
#define LED_EL_TRIGGER_INTERVAL 500 

typedef struct {
  char*  protocol;
  char*  hdVersion;
  char*  fwVersion;
  char*  rfVersion;
} OTA_Versions_t;

OSStatus startEasyLink( mico_Context_t * const inContext);

OSStatus stopEasyLink( mico_Context_t * const inContext);

OSStatus FTC_AddSector(json_object* sectorArray, char* const name,  json_object *menuArray);

OSStatus FTC_AddStringCellToSector(json_object* sector, char* const name,  char* const content, char* const privilege, json_object* secectionArray);

OSStatus FTC_AddNumberCellToSector(json_object* sector, char* const name,  int content, char* const privilege, json_object* secectionArray);

OSStatus FTC_AddSwitchCellToSector(json_object* sector, char* const name,  boolean b, char* const privilege);

OSStatus FTC_AddMenuCellToSector(json_object* sector, char* const name, json_object* lowerSectorArray);

OSStatus FTC_AddTopMenu(json_object **topMenu, char* const name, json_object* lowerSectorArray, OTA_Versions_t versions);

#endif


