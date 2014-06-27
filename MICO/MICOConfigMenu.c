/**
  ******************************************************************************
  * @file    MICOConfigMenu.c 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   This file provide function for createing configration menu that can 
  *          be displayed on EasyLink APP on iOS or Android.
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

#include "Debug.h"
#include "external/JSON-C/json.h"
#include "MICOConfigMenu.h"

OSStatus MICOAddSector(json_object* sectorArray, char* const name,  json_object *menuArray)
{
  OSStatus err;
  json_object *object;
  err = kNoErr;

  object = json_object_new_object();
  require_action(object, exit, err = kNoMemoryErr);
  json_object_object_add(object, "N", json_object_new_string(name));      
  json_object_object_add(object, "C", menuArray);
  json_object_array_add(sectorArray, object);

exit:
  return err;
}

OSStatus MICOAddStringCellToSector(json_object* sector, char* const name,  char* const content, char* const privilege, json_object* secectionArray)
{
  OSStatus err;
  json_object *object;
  err = kNoErr;

  object = json_object_new_object();
  require_action(object, exit, err = kNoMemoryErr);
  json_object_object_add(object, "N", json_object_new_string(name));      
  json_object_object_add(object, "C", json_object_new_string(content));
  json_object_object_add(object, "P", json_object_new_string(privilege)); 

  if(secectionArray)
    json_object_object_add(object, "S", secectionArray); 

  json_object_array_add(sector, object);


exit:
  return err;
}

OSStatus MICOAddNumberCellToSector(json_object* sector, char* const name,  int content, char* const privilege, json_object* secectionArray)
{
  OSStatus err;
  json_object *object;
  err = kNoErr;

  object = json_object_new_object();
  require_action(object, exit, err = kNoMemoryErr);
  json_object_object_add(object, "N", json_object_new_string(name));      

  json_object_object_add(object, "C", json_object_new_int(content));
  json_object_object_add(object, "P", json_object_new_string(privilege)); 

  if(secectionArray)
    json_object_object_add(object, "S", secectionArray); 

  json_object_array_add(sector, object);

exit:
  return err;
}

OSStatus MICOAddSwitchCellToSector(json_object* sector, char* const name,  boolean b, char* const privilege)
{
  OSStatus err;
  json_object *object;
  err = kNoErr;

  object = json_object_new_object();
  require_action(object, exit, err = kNoMemoryErr);
  json_object_object_add(object, "N", json_object_new_string(name));      
  json_object_object_add(object, "C", json_object_new_boolean(b));
  json_object_object_add(object, "P", json_object_new_string(privilege)); 
  json_object_array_add(sector, object);

exit:
  return err;
}

OSStatus MICOAddMenuCellToSector(json_object* sector, char* const name, json_object* lowerSectorArray)
{
  OSStatus err;
  json_object *object;
  err = kNoErr;

  object = json_object_new_object();
  require_action(object, exit, err = kNoMemoryErr);
  json_object_object_add(object, "N", json_object_new_string(name));
  json_object_object_add(object, "C", lowerSectorArray);
  json_object_array_add(sector, object);

exit:
  return err;
}

OSStatus MICOAddTopMenu(json_object **outTopMenu, char* const inName, json_object* inlowerSectorArray, OTA_Versions_t inVersions)
{
  OSStatus err;
  json_object *object;
  err = kNoErr;
  require_action(inVersions.protocol, exit, err = kParamErr);
  require_action(inVersions.hdVersion, exit, err = kParamErr);
  require_action(inVersions.fwVersion, exit, err = kParamErr);

  object = json_object_new_object();
  require_action(object, exit, err = kNoMemoryErr);
  json_object_object_add(object, "N", json_object_new_string(inName));
  json_object_object_add(object, "C", inlowerSectorArray);

  json_object_object_add(object, "PO", json_object_new_string(inVersions.protocol));
  json_object_object_add(object, "HD", json_object_new_string(inVersions.hdVersion));
  json_object_object_add(object, "FW", json_object_new_string(inVersions.fwVersion));
  if(inVersions.rfVersion)
    json_object_object_add(object, "RF", json_object_new_string(inVersions.rfVersion));
 
  *outTopMenu = object;
exit:
  return err;
}

