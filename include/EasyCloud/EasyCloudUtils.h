/**
  ******************************************************************************
  * @file    EasyCloudUtils.h 
  * @author  Eshen.Wang
  * @version V1.0.0
  * @date    29-Nov-2014
  * @brief   This header contains function prototypes for some utils functions.
  * @NOTE    These are support functions not supplied in MICO support, could be
  *          removed if MICO has these utils.
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

#ifndef __EASYCLOUD_UTILS_H__
#define __EASYCLOUD_UTILS_H__

//#include <stdarg.h>
#include "Common.h"

#include "HTTPUtils.h"

// ============================= HTTP utils ================================

#define kHTTPGetMethod      "GET"

#define kMIMEType_EASYCLOUD_OTA         "application/octet-stream"

OSStatus CreateHTTPMessageEx( const char *methold, const char * host, 
                             const char *url, const char *contentType, 
                             uint8_t *inData, size_t inDataLen, 
                             uint8_t **outMessage, size_t *outMessageSize );
OSStatus CreateHTTPMessageWithRange( const char *methold, const char * host, 
                             const char *url, const char *contentType, 
                             uint64_t rangeStart,
                             uint8_t *inData, size_t inDataLen, 
                             uint8_t **outMessage, size_t *outMessageSize );

OSStatus CreateSimpleHTTPFailedMessage( uint8_t **outMessage, 
                                       size_t *outMessageSize );
int SocketReadHTTPHeaderEx( int inSock, HTTPHeader_t *inHeader );
OSStatus SocketReadHTTPBodyEx( int inSock, HTTPHeader_t *inHeader );

// ============================= STRING UTILS ==============================

char* DataToHexStringLowercase( const uint8_t *inBuf, size_t inBufLen );
// string replace, dst string must be freed by user
char* str_replace(char *dst, const char *src, const unsigned int srcLen, char *old_str, char *new_str);

#ifdef MICO_FLASH_FOR_UPDATE
uint32_t getFlashStorageAddress(void);
#endif

#endif // __EASYCLOUD_UTILS_H__

