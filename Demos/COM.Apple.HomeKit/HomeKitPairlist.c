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
#include "platform_config.h"

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

OSStatus HKInsertPairInfo(char controllerIdentifier[64], uint8_t controllerLTPK[32], bool admin)
{
  OSStatus err = kNoErr;
  pair_list_in_flash_t        *pairList = NULL;
  uint32_t i;

  /* Pair storage */
  pairList = calloc(1, sizeof(pair_list_in_flash_t));
  require_action(pairList, exit, err = kNoMemoryErr);
  err = HMReadPairList(pairList);
  require_noerr(err, exit);
  
  /* Looking for controller pair record */
  for(i=0; i<MAXPairNumber; i++){
    if(strncmp(pairList->pairInfo[i].controllerName, controllerIdentifier, 64)==0)
      break;
  }

  /* This is a new record */
  if(i == MAXPairNumber){
    for(i=0; i<MAXPairNumber; i++){
      if(pairList->pairInfo[i].controllerName[0] == 0x0)
      break;
    }
  }
  
  /* No space for new record */
  require_action(i < MAXPairNumber, exit, err = kNoSpaceErr);

  /* Write pair info to flash */
  strcpy(pairList->pairInfo[i].controllerName, controllerIdentifier);
  memcpy(pairList->pairInfo[i].controllerLTPK, controllerLTPK, 32);
  if(admin)
    pairList->pairInfo[i].permission = pairList->pairInfo[i].permission|0x00000001;
  else
    pairList->pairInfo[i].permission = pairList->pairInfo[i].permission&0xFFFFFFFE;
  err = HMUpdatePairList(pairList);

exit: 
  if(pairList) free(pairList);
  return err;
}


uint8_t foundControllerLTPK[32];

uint8_t * HMFindLTPK(char * name)
{
  uint8_t *controllerLTPK = NULL;
  uint32_t i;

  pair_list_in_flash_t *pairList = malloc(sizeof(pair_list_in_flash_t));
  HMReadPairList(pairList);

  for(i=0; i<MAXPairNumber; i++){
    if(strncmp(pairList->pairInfo[i].controllerName, name, 64) == 0){
      memcpy(foundControllerLTPK, pairList->pairInfo[i].controllerLTPK, 32);
      controllerLTPK = foundControllerLTPK;
      break;
    }
  }
  
  free(pairList);
  return controllerLTPK;
}

bool HMFindAdmin(char * name)
{
  uint32_t i;
  bool ret = false;

  pair_list_in_flash_t *pairList = malloc(sizeof(pair_list_in_flash_t));
  HMReadPairList(pairList);

  for(i=0; i<MAXPairNumber; i++){
    if(strncmp(pairList->pairInfo[i].controllerName, name, 64) == 0){
      ret = pairList->pairInfo[i].permission&0x1;
      free(pairList);
      return ret;
    }
  }
  
  free(pairList);
  return ret;
}

OSStatus HMRemoveLTPK(char * name)
{
  uint32_t i;
  OSStatus err = kNoErr;

  pair_list_in_flash_t *pairList = malloc(sizeof(pair_list_in_flash_t));
  HMReadPairList(pairList);

  for(i=0; i<MAXPairNumber; i++){
    if(strncmp(pairList->pairInfo[i].controllerName, name, 64) == 0){
      pairList->pairInfo[i].controllerName[0] = 0x0; //Clear the controller name record
      err = HMUpdatePairList(pairList);
      break;
    }
  }
  
  free(pairList);
  return err;  
}



