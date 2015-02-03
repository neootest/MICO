/**
******************************************************************************
* @file    MicoDriverWdg.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide WDG driver functions.
******************************************************************************
*
*  The MIT License
*  Copyright (c) 2014 MXCHIP Inc.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy 
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights 
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is furnished
*  to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
*  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************
*/ 


#include "MICOPlatform.h"
#include "MICORTOS.h"
#include "common.h"
#include "debug.h"

#include "platform.h"
#include "platform_common_config.h"

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/
#ifndef MICO_DISABLE_WATCHDOG
static __IO uint32_t LsiFreq = 0;
static __IO uint32_t CaptureNumber = 0, PeriodValue = 0;
static mico_semaphore_t  _measureLSIComplete_SEM = NULL;
uint16_t tmpCC4[2] = {0, 0};
#endif

/******************************************************
 *               Function Declarations
 ******************************************************/
#ifndef MICO_DISABLE_WATCHDOG
static uint32_t GetLSIFrequency(void);
#endif

/******************************************************
 *               Function Definitions
 ******************************************************/

OSStatus MicoWdgInitialize( uint32_t timeout_ms )
{
// PLATFORM_TO_DO
#ifndef MICO_DISABLE_WATCHDOG
  retur kUnsupportedErr
#else
  UNUSED_PARAMETER( timeout_ms );
  return kUnsupportedErr;
#endif
}

OSStatus MicoWdgFinalize( void )
{
    // PLATFORM_TO_DO
    return kNoErr;
}

void MicoWdgReload( void )
{
#ifndef MICO_DISABLE_WATCHDOG
  return;
#else
  return;
#endif
}


