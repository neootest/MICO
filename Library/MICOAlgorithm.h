/**
  ******************************************************************************
  * @file    MICOAlgorithm.h 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   This file provides all the headers of Algorithms provided by MICO.
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

#ifndef __MICO_ALGORITHM_H_
#define __MICO_ALGORITHM_H_

#include "Common.h"

typedef struct
{
    uint32_t total[2];
    uint32_t state[4];
    uint8_t  buffer[64];
    uint8_t  ipad[64];
    uint8_t  opad[64];
} md5_context;


/**
 * @brief          MD5 context setup
 *
 * @param ctx      context to be initialized
 */
void md5_starts( md5_context *ctx );

/**
 * @brief          MD5 process buffer
 *
 * @param ctx      MD5 context
 * @param input    buffer holding the  data
 * @param ilen     length of the input data
 */
void md5_update( md5_context *ctx, unsigned char *input, int ilen );

/**
 * @brief          MD5 final digest
 *
 * @param ctx      MD5 context
 * @param output   MD5 checksum result
 */
void md5_finish( md5_context *ctx, unsigned char output[16] );


#endif /* __MICO_ALGORITHM_H_ */

