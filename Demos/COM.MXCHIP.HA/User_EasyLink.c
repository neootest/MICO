#include "Common.h"
#include "debug.h"
#include "platform.h"
#include "EasyLink.h"
#include "external/JSON-C/json.h"
#include "mico_define.h"
#include "StringUtils.h"

#define user_easylink_log(M, ...) custom_log("User EasyLink", M, ##__VA_ARGS__)
#define user_easylink_log_trace() custom_log_trace("User EasyLink")

void EasyLinkWillStart( mico_Context_t * const inContext )
{
  user_easylink_log_trace();
  (void)(inContext); 
  return;
}

void EasyLinkWillStop( mico_Context_t * const inContext )
{
   user_easylink_log_trace();
  (void)(inContext); 
  return;
}

OSStatus EasyLinkRecvAuthData(char * userInfo, mico_Context_t * const inContext )
{
  user_easylink_log_trace();
  (void)(inContext);
  (void)(userInfo);
  return kNoErr;
}


OSStatus EasyLinkCreateReportJsonHTTPMessage( mico_Context_t * const inContext )
{
  OSStatus err = kNoErr;
  user_easylink_log_trace();
  char name[50], *tempString;
  OTA_Versions_t versions;

  if(inContext->flashContentInRam.micoSystemConfig.configured == wLanUnConfigured){
    /*You can upload a specific menu*/
  }

  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  snprintf(name, 50, "%s(%c%c%c%c%c%c)",inContext->flashContentInRam.micoSystemConfig.model, 
                                        inContext->micoStatus.mac[9],  inContext->micoStatus.mac[10], 
                                        inContext->micoStatus.mac[12], inContext->micoStatus.mac[13],
                                        inContext->micoStatus.mac[15], inContext->micoStatus.mac[16]);

  versions.fwVersion = inContext->flashContentInRam.micoSystemConfig.firmwareRevision;
  versions.hdVersion = inContext->flashContentInRam.micoSystemConfig.hardwareRevision;
  versions.protocol = inContext->flashContentInRam.micoSystemConfig.protocol;
  versions.rfVersion = NULL;

  json_object *sectors, *sector, *subMenuSectors, *subMenuSector, *mainObject;

  sectors = json_object_new_array();
  require( sectors, exit );

  err = FTC_AddTopMenu(&mainObject, name, sectors, versions);

  require_noerr(err, exit);

  /*Sector 1*/
  sector = json_object_new_array();
  require( sector, exit );
  err = FTC_AddSector(sectors, "MICO SYSTEM",    sector);
  require_noerr(err, exit);

    /*name cell*/
    err = FTC_AddStringCellToSector(sector, "Device Name",    inContext->flashContentInRam.micoSystemConfig.name,               "RW", NULL);
    require_noerr(err, exit);

    //Bonjour switcher cell
    err = FTC_AddSwitchCellToSector(sector, "Bonjour",        inContext->flashContentInRam.micoSystemConfig.bonjourEnable,      "RW");
    require_noerr(err, exit);

    //RF power save switcher cell
    err = FTC_AddSwitchCellToSector(sector, "RF power save",  inContext->flashContentInRam.micoSystemConfig.rfPowerSaveEnable,  "RW");
    require_noerr(err, exit);

    //MCU power save switcher cell
    err = FTC_AddSwitchCellToSector(sector, "MCU power save", inContext->flashContentInRam.micoSystemConfig.mcuPowerSaveEnable, "RW");
    require_noerr(err, exit);

    /*sub menu*/
    subMenuSectors = json_object_new_array();
    require( subMenuSectors, exit );
    err = FTC_AddMenuCellToSector(sector, "Detail", subMenuSectors);
    require_noerr(err, exit);
      
      subMenuSector = json_object_new_array();
      require( subMenuSector, exit );
      err = FTC_AddSector(subMenuSectors,  "",    subMenuSector);
      require_noerr(err, exit);

        err = FTC_AddStringCellToSector(subMenuSector, "Firmware Rev.",  inContext->flashContentInRam.micoSystemConfig.firmwareRevision, "RO", NULL);
        require_noerr(err, exit);
        err = FTC_AddStringCellToSector(subMenuSector, "Hardware Rev.",  inContext->flashContentInRam.micoSystemConfig.hardwareRevision, "RO", NULL);
        require_noerr(err, exit);
        err = FTC_AddStringCellToSector(subMenuSector, "MICO OS Rev.",   system_lib_version(),             "RO", NULL);
        require_noerr(err, exit);
        err = FTC_AddStringCellToSector(subMenuSector, "Model",          inContext->flashContentInRam.micoSystemConfig.model,            "RO", NULL);
        require_noerr(err, exit);
        err = FTC_AddStringCellToSector(subMenuSector, "Manufacturer",   inContext->flashContentInRam.micoSystemConfig.manufacturer,     "RO", NULL);
        require_noerr(err, exit);
        err = FTC_AddStringCellToSector(subMenuSector, "Protocol",       inContext->flashContentInRam.micoSystemConfig.protocol,         "RO", NULL);
        require_noerr(err, exit);

      subMenuSector = json_object_new_array();
      err = FTC_AddSector(subMenuSectors,  "WLAN",    subMenuSector);
      require_noerr(err, exit);

        err = FTC_AddStringCellToSector(subMenuSector, "Wi-Fi",        inContext->flashContentInRam.micoSystemConfig.ssid,     "RO", NULL);
        require_noerr(err, exit);

        err = FTC_AddStringCellToSector(subMenuSector, "Password",     inContext->flashContentInRam.micoSystemConfig.user_key, "RO", NULL);
        require_noerr(err, exit);

        tempString = DataToHexStringWithColons( (uint8_t *)inContext->flashContentInRam.micoSystemConfig.bssid, 6 );
        err = FTC_AddStringCellToSector(subMenuSector, "BSSID",        tempString, "RO", NULL);
        require_noerr(err, exit);
        free(tempString);

        err = FTC_AddNumberCellToSector(subMenuSector, "Channel",      inContext->flashContentInRam.micoSystemConfig.channel, "RO", NULL);
        require_noerr(err, exit);

        switch(inContext->flashContentInRam.micoSystemConfig.security){
          case SECURITY_TYPE_NONE:
            err = FTC_AddStringCellToSector(subMenuSector, "Security",   "Open system", "RO", NULL); 
            break;
          case SECURITY_TYPE_WEP:
            err = FTC_AddStringCellToSector(subMenuSector, "Security",   "WEP",         "RO", NULL); 
            break;
          case SECURITY_TYPE_WPA_TKIP:
            err = FTC_AddStringCellToSector(subMenuSector, "Security",   "WPA TKIP",    "RO", NULL); 
            break;
          case SECURITY_TYPE_WPA_AES:
            err = FTC_AddStringCellToSector(subMenuSector, "Security",   "WPA AES",     "RO", NULL); 
            break;
          case SECURITY_TYPE_WPA2_TKIP:
            err = FTC_AddStringCellToSector(subMenuSector, "Security",   "WPA2 TKIP",   "RO", NULL); 
            break;
          case SECURITY_TYPE_WPA2_AES:
            err = FTC_AddStringCellToSector(subMenuSector, "Security",   "WPA2 AES",    "RO", NULL); 
            break;
          case SECURITY_TYPE_WPA2_MIXED:
            err = FTC_AddStringCellToSector(subMenuSector, "Security",   "WPA2 MIXED",  "RO", NULL); 
            break;
          default:
            err = FTC_AddStringCellToSector(subMenuSector, "Security",   "Auto",      "RO", NULL); 
            break;
        }
        require_noerr(err, exit); 

        if(inContext->flashContentInRam.micoSystemConfig.keyLength == maxKeyLen){ /*This is a PMK key, generated by user key in WPA security type*/
          tempString = calloc(maxKeyLen+1, 1);
          require_action(tempString, exit, err=kNoMemoryErr);
          memcpy(tempString, inContext->flashContentInRam.micoSystemConfig.key, maxKeyLen);
          err = FTC_AddStringCellToSector(subMenuSector, "PMK",          tempString, "RO", NULL);
          require_noerr(err, exit);
          free(tempString);
        }
        else{
          err = FTC_AddStringCellToSector(subMenuSector, "KEY",          inContext->flashContentInRam.micoSystemConfig.user_key,  "RO", NULL);
          require_noerr(err, exit);
        }

        /*DHCP cell*/
        err = FTC_AddSwitchCellToSector(subMenuSector, "DHCP",        inContext->flashContentInRam.micoSystemConfig.dhcpEnable,   "RO");
        require_noerr(err, exit);
        /*Local cell*/
        err = FTC_AddStringCellToSector(subMenuSector, "IP address",  inContext->micoStatus.localIp,   "RO", NULL);
        require_noerr(err, exit);
        /*Netmask cell*/
        err = FTC_AddStringCellToSector(subMenuSector, "Net Mask",    inContext->micoStatus.netMask,   "RO", NULL);
        require_noerr(err, exit);
        /*Gateway cell*/
        err = FTC_AddStringCellToSector(subMenuSector, "Gateway",     inContext->micoStatus.gateWay,   "RO", NULL);
        require_noerr(err, exit);
        /*DNS server cell*/
        err = FTC_AddStringCellToSector(subMenuSector, "DNS Server",  inContext->micoStatus.dnsServer, "RO", NULL);
        require_noerr(err, exit);

  /*Sector 3*/
  sector = json_object_new_array();
  require( sector, exit );
  err = FTC_AddSector(sectors, "HA services",           sector);
  require_noerr(err, exit);


    // HA protocol remote server connection enable
    err = FTC_AddSwitchCellToSector(sector, "Connect HA Server",   inContext->flashContentInRam.appConfig.remoteServerEnable,   "RW");
    require_noerr(err, exit);

    //Seerver address cell
    err = FTC_AddStringCellToSector(sector, "HA Server",           inContext->flashContentInRam.appConfig.remoteServerDomain,   "RW", NULL);
    require_noerr(err, exit);

    //Seerver port cell
    err = FTC_AddNumberCellToSector(sector, "HA Server Port",      inContext->flashContentInRam.appConfig.remoteServerPort,   "RW", NULL);
    require_noerr(err, exit);

    // //Tencent cloud switcher cell
    // err = FTC_AddSwitchCellToSector(sector, "Tencent cloud",    inContext->flashContentInRam.micoSystemConfig.bonjourEnable,   "RW");
    // require_noerr(err, exit);

    // //JD smart switcher cell
    // err = FTC_AddSwitchCellToSector(sector, "JD smart",         inContext->flashContentInRam.micoSystemConfig.bonjourEnable,   "RW");
    // require_noerr(err, exit);

  /*Sector 5*/
  sector = json_object_new_array();
  require( sector, exit );
  err = FTC_AddSector(sectors, "MCU IOs",            sector);
  require_noerr(err, exit);



    /*UART Baurdrate cell*/
    json_object *selectArray;
    selectArray = json_object_new_array();
    require( selectArray, exit );
    json_object_array_add(selectArray, json_object_new_int(9600));
    json_object_array_add(selectArray, json_object_new_int(19200));
    json_object_array_add(selectArray, json_object_new_int(38400));
    json_object_array_add(selectArray, json_object_new_int(57600));
    json_object_array_add(selectArray, json_object_new_int(115200));
    err = FTC_AddNumberCellToSector(sector, "Baurdrate", inContext->flashContentInRam.appConfig.USART_BaudRate, "RW", selectArray);
    require_noerr(err, exit);

  inContext->micoStatus.easylink_report = mainObject;
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
  
exit:
    return err;

}

OSStatus EasyLinkIncommingJsonHTTPMessage( const char *input, mico_Context_t * const inContext )
{
  OSStatus err = kNoErr;
  json_object *new_obj;
  new_obj = json_tokener_parse(input);
  require_action(new_obj, exit, err = kUnknownErr);
  user_easylink_log("Recv config object=%s", json_object_to_json_string(new_obj));
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  json_object_object_foreach(new_obj, key, val) {
    if(!strcmp(key, "Device Name")){
      strncpy(inContext->flashContentInRam.micoSystemConfig.name, json_object_get_string(val), maxNameLen);
    }else if(!strcmp(key, "RF power save")){
      inContext->flashContentInRam.micoSystemConfig.rfPowerSaveEnable = json_object_get_boolean(val);
    }else if(!strcmp(key, "MCU power save")){
      inContext->flashContentInRam.micoSystemConfig.mcuPowerSaveEnable = json_object_get_boolean(val);
    }else if(!strcmp(key, "Bonjour")){
      inContext->flashContentInRam.micoSystemConfig.bonjourEnable = json_object_get_boolean(val);
    }else if(!strcmp(key, "Connect HA Server")){
      inContext->flashContentInRam.appConfig.remoteServerEnable = json_object_get_boolean(val);
    }else if(!strcmp(key, "HA Server")){
      strncpy(inContext->flashContentInRam.appConfig.remoteServerDomain, json_object_get_string(val), 64);
    }else if(!strcmp(key, "HA Server Port")){
      inContext->flashContentInRam.appConfig.remoteServerPort = json_object_get_int(val);
    }else if(!strcmp(key, "Baurdrate")){
      inContext->flashContentInRam.appConfig.USART_BaudRate = json_object_get_int(val);
    }
  }
  json_object_put(new_obj);
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);

  inContext->flashContentInRam.micoSystemConfig.configured = allConfigured;
  mico_UpdateConfiguration(inContext);

exit:
  return err; 
}