/***
 * @File ap80xx_flash.h
 * @brief a patch of flash 
 * @author Jerry Yu
 * @Date Jan 26th, 2015
 * */

#ifndef __AP80XX_FLASH_H__
#define __AP80XX_FLASH_H__
#include "spi_flash.h"

typedef enum
{ 
  FLASH_BUSY = 1,
  FLASH_ERROR_PGS,
  FLASH_ERROR_PGP,
  FLASH_ERROR_PGA,
  FLASH_ERROR_WRP,
  FLASH_ERROR_PROGRAM,
  FLASH_ERROR_OPERATION,
  FLASH_COMPLETE
}FLASH_Status;

void FLASH_Unlock(void);
void FLASH_Lock(void);
FLASH_Status FLASH_ProgramByte(uint32_t Address, uint8_t Data);
void FLASH_ClearFlag(uint32_t FLASH_FLAG);

#endif

