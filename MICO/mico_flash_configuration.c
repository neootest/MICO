/**
  ******************************************************************************
  * @file    mico_flash_configuration.c 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   This file provide constant operations in nonvolatile memory.
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

#include "mico_define.h"
#include "mico_api.h"
#include "PlatformFlash.h"
#include "Platform.h"

/* Update seed number every time*/
static int32_t seedNum = 0;

__weak void appRestoreDefault_callback(mico_Context_t *inContext)
{

}

OSStatus mico_RestoreDefault(mico_Context_t *inContext)
{ 
  OSStatus err = kNoErr;
  uint32_t paraStartAddress, paraEndAddress;
 
  paraStartAddress = PARA_START_ADDRESS;
  paraEndAddress = PARA_END_ADDRESS;

  /*wlan configration is not need to change to a default state, use easylink to do that*/
  sprintf(inContext->flashContentInRam.micoSystemConfig.name, DEFAULT_NAME);
  sprintf(inContext->flashContentInRam.micoSystemConfig.firmwareRevision, FIRMWARE_REVISION);
  sprintf(inContext->flashContentInRam.micoSystemConfig.hardwareRevision, HARDWARE_REVISION);
  sprintf(inContext->flashContentInRam.micoSystemConfig.model, MODEL);
  sprintf(inContext->flashContentInRam.micoSystemConfig.manufacturer, MANUFACTURER);
  sprintf(inContext->flashContentInRam.micoSystemConfig.protocol, PROTOCOL);
  inContext->flashContentInRam.micoSystemConfig.configured = unConfigured;
  inContext->flashContentInRam.micoSystemConfig.rfPowerSaveEnable = false;
  inContext->flashContentInRam.micoSystemConfig.mcuPowerSaveEnable = false;
  inContext->flashContentInRam.micoSystemConfig.bonjourEnable = true;
  inContext->flashContentInRam.micoSystemConfig.seed = seedNum;

  /*Application's default configuration*/
  inContext->flashContentInRam.appConfig.configDataVer = CONFIGURATION_VERSION;
  inContext->flashContentInRam.appConfig.localServerPort = LOCAL_PORT;
  appRestoreDefault_callback(inContext);

  err = PlatformFlashInitialize();
  require_noerr(err, exit);
  err = PlatformFlashErase(paraStartAddress, paraEndAddress);
  require_noerr(err, exit);
  err = PlatformFlashWrite(&paraStartAddress, (void *)inContext, sizeof(flash_content_t));
  require_noerr(err, exit);
  err = PlatformFlashFinalize();
  require_noerr(err, exit);

exit:
  return err;
}

OSStatus mico_ReadConfiguration(mico_Context_t *inContext)
{
  uint32_t configInFlash;
  OSStatus err = kNoErr;
  configInFlash = PARA_START_ADDRESS;
  memcpy(&inContext->flashContentInRam, (void *)configInFlash, sizeof(flash_content_t));
  seedNum = inContext->flashContentInRam.micoSystemConfig.seed;
  if(seedNum == -1) seedNum = 0;

  if(inContext->flashContentInRam.appConfig.configDataVer != CONFIGURATION_VERSION){
    err = mico_RestoreDefault(inContext);
    require_noerr(err, exit);
    PlatformSoftReboot();
  }

  if(inContext->flashContentInRam.micoSystemConfig.dhcpEnable == DHCP_Disable){
    strcpy((char *)inContext->micoStatus.localIp, inContext->flashContentInRam.micoSystemConfig.localIp);
    strcpy((char *)inContext->micoStatus.netMask, inContext->flashContentInRam.micoSystemConfig.netMask);
    strcpy((char *)inContext->micoStatus.gateWay, inContext->flashContentInRam.micoSystemConfig.gateWay);
    strcpy((char *)inContext->micoStatus.dnsServer, inContext->flashContentInRam.micoSystemConfig.dnsServer);
  }

exit: 
  return err;
}

OSStatus mico_UpdateConfiguration(mico_Context_t *inContext)
{
  OSStatus err = kNoErr;
  uint32_t paraStartAddress, paraEndAddress;
 
  paraStartAddress = PARA_START_ADDRESS;
  paraEndAddress = PARA_END_ADDRESS;

  inContext->flashContentInRam.micoSystemConfig.seed = ++seedNum;
  err = PlatformFlashInitialize();
  require_noerr(err, exit);
  err = PlatformFlashErase(paraStartAddress, paraEndAddress);
  require_noerr(err, exit);
  err = PlatformFlashWrite(&paraStartAddress, (u32 *)&inContext->flashContentInRam, sizeof(flash_content_t));
  require_noerr(err, exit);
  err = PlatformFlashFinalize();
  require_noerr(err, exit);

exit:
  return err;
}


