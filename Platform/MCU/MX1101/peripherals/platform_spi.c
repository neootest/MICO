/**
******************************************************************************
* @file    MicoDriverSpi.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide SPI driver functions.
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


#include "platform.h"
#include "platform_peripheral.h"

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

/******************************************************
*               Function Declarations
******************************************************/

/******************************************************
*               Function Definitions
******************************************************/

OSStatus platform_spi_init( const platform_spi_t* spi, const platform_spi_config_t* config )
{
  UNUSED_PARAMETER(spi);
  UNUSED_PARAMETER(config);
  platform_log("unimplemented");
  return kUnsupportedErr;
}

OSStatus platform_spi_deinit( const platform_spi_t* spi )
{
  UNUSED_PARAMETER( spi );
  platform_log("unimplemented");
  return kUnsupportedErr;
}

OSStatus platform_spi_transfer( const platform_spi_t* spi, const platform_spi_config_t* config, const platform_spi_message_segment_t* segments, uint16_t number_of_segments )
{
  UNUSED_PARAMETER(spi);
  UNUSED_PARAMETER(config);
  UNUSED_PARAMETER(segments);
  UNUSED_PARAMETER(number_of_segments);
  platform_log("unimplemented");
  return kUnsupportedErr;
}



