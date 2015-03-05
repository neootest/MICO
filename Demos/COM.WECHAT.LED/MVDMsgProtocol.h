/**
******************************************************************************
* @file    LM_LEDCmd.h 
* @author  Eshen Wang
* @version V0.0.1
* @date    29-Nov-2014
* @brief   This header contains the cmd interfaces 
*          of LiMu smart LED USART protocol. 
  operation
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

#ifndef __LM_LED_CMD_H_
#define __LM_LED_CMD_H_


#include "MICODefine.h"


/*******************************************************************************
 * USER CODE
 ******************************************************************************/

// cmd string len for json
#define MAX_SIZE_CMD         16

// frame format
#define LED_USART_MSG_HEAD   0xAA
#define LED_USART_MSG_TAIL   0x55

//cmd code
#define LED_CMD_ON           0xA5
#define LED_CMD_OFF          0xA0
#define LED_CMD_DELAY        0xA2
#define LED_CMD_STATUS       0xA8

//response code
#define LED_RESP_SET_OK      0xB0
#define LED_RESP_STATUS      0xB2

//response value
#define LED_RESP_CMD_VALUE_OK       "OK"
#define LED_RESP_CMD_VALUE_FAILED   "FAILED"
#define LED_RESP_CMD_VALUE_STATUS   "status"


typedef struct _lm_usart_message_t {
  uint8_t head;
  uint8_t cmd;
  uint8_t param1;
  uint8_t param2;
  uint8_t param3;
  uint8_t checksum;
  uint8_t tail;
}lm_usart_message_t;


//user json
#define LED_PARAM_WHITE         "white"
#define LED_PARAM_YELLOW        "yellow"
#define LED_PARAM_BRIGHTNESS    "brightness"

#define LED_PARAM_HOUR          "hour"
#define LED_PARAM_MINUTE        "minute"
#define LED_PARAM_SECOND        "second"


/*******************************************************************************
 * INTERFACES PROTOTYPE
 ******************************************************************************/

OSStatus MVDMsgTransformCloud2Device(unsigned char* inJsonString, unsigned int inJsonStringLen, 
                               unsigned char** outUsartCmd, unsigned int* outUsartCmdLen);
 
OSStatus MVDMsgTransformDevice2Cloud(unsigned char* inUsartString, unsigned int inUsartStringLen, 
                              unsigned char** outJson, unsigned int* outJsonLen);

bool LedControlMsgHandler(unsigned char *Msg, unsigned int len);

#endif
