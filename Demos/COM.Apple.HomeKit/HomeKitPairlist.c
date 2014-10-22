/**
  ******************************************************************************
  * @file    MICOPARASTORAGE.c 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   This file provide operations on nonvolatile memory.
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

#include "HomeKitPairlist.h"
#include "Debug.h"
#include "MicoPlatform.h"
#include "platform_common_config.h"

/* Update seed number every time*/

OSStatus HMClearPairList(void)
{ 
  OSStatus err = kNoErr;
  uint32_t exParaStartAddress, exParaEndAddress;
 
  exParaStartAddress = EX_PARA_START_ADDRESS;
  exParaEndAddress = EX_PARA_END_ADDRESS;
  pair_list_in_flash_t *pairList = NULL;
  pairList = calloc(1, sizeof(pair_list_in_flash_t));
  require_action(pairList, exit, err = kNoMemoryErr);

  err = MicoFlashInitialize(MICO_FLASH_FOR_EX_PARA);
  require_noerr(err, exit);
  err = MicoFlashErase(MICO_FLASH_FOR_EX_PARA, exParaStartAddress, exParaEndAddress);
  require_noerr(err, exit);
  err = MicoFlashWrite(MICO_FLASH_FOR_EX_PARA, &exParaStartAddress, (uint8_t *)pairList, sizeof(pair_list_in_flash_t));
  require_noerr(err, exit);
  err = MicoFlashFinalize(MICO_FLASH_FOR_EX_PARA);
  require_noerr(err, exit);

exit:
  if(pairList) free(pairList);
  return err;
}

OSStatus HMReadPairList(pair_list_in_flash_t *pPairList)
{
  uint32_t configInFlash;
  OSStatus err = kNoErr;
  require(pPairList, exit);

  configInFlash = EX_PARA_START_ADDRESS;
  err = MicoFlashRead(MICO_FLASH_FOR_EX_PARA, &configInFlash, (uint8_t *)pPairList, sizeof(pair_list_in_flash_t));
  //memcpy(pPairList, (void *)configInFlash, sizeof(pair_list_in_flash_t));

exit: 
  return err;
}

OSStatus HMUpdatePairList(pair_list_in_flash_t *pPairList)
{
  OSStatus err = kNoErr;
  uint32_t exParaStartAddress, exParaEndAddress;
 
  exParaStartAddress = EX_PARA_START_ADDRESS;
  exParaEndAddress = EX_PARA_END_ADDRESS;

  err = MicoFlashInitialize(MICO_FLASH_FOR_EX_PARA);
  require_noerr(err, exit);
  err = MicoFlashErase(MICO_FLASH_FOR_EX_PARA, exParaStartAddress, exParaEndAddress);
  require_noerr(err, exit);
  err = MicoFlashWrite(MICO_FLASH_FOR_EX_PARA, &exParaStartAddress, (uint8_t *)pPairList, sizeof(pair_list_in_flash_t));
  require_noerr(err, exit);
  err = MicoFlashFinalize(MICO_FLASH_FOR_EX_PARA);
  require_noerr(err, exit);

exit:
  return err;
}

static uint8_t foundControllerLTPK[32];
uint8_t * HMFindLTPK(char * name)
{
  int i;

  //pair_list_in_flash_t *pairList = (pair_list_in_flash_t *)EX_PARA_START_ADDRESS;
  pair_list_in_flash_t pairList;
  HMReadPairList(&pairList);

  for(i=0; i<MAXPairNumber; i++){
    if(strncmp(pairList.pairInfo[i].controllerName, name, 64) == 0)
      memcpy(foundControllerLTPK, pairList.pairInfo[i].controllerLTPK, 32);
      return foundControllerLTPK;
  }
  return NULL;

}



