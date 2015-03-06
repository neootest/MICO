/**
******************************************************************************
* @file    LM_LEDCmd.c 
* @author  Eshen Wang
* @version V0.0.1
* @date    29-Nov-2014
* @brief   This file contains the usart cmd  
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

#include "MICODefine.h"
#include "JSON-C/json.h"
#include "MVDMsgProtocol.h"
#include "RGB.h"
#include "StringUtils.h"

#define led_log(M, ...) custom_log("LED", M, ##__VA_ARGS__)
#define led_log_trace() custom_log_trace("LED")


/*******************************************************************************
 * USER CODE
 ******************************************************************************/

static int getCmdNum(char* cmd, unsigned int len)
{
  //led_log("cmd[%d]=[%s]", len, cmd);
  if(!strncmp(cmd, "on", len))
    return 1;
  if(!strncmp(cmd, "off", len))
    return 2;
  if(!strncmp(cmd, "delay", len))
    return 3;
  if(!strncmp(cmd, "status", len))
    return 4;
  
  return -1;
}

static uint8_t checksum(lm_usart_message_t msg)
{
  uint8_t result = 0x0;
  uint32_t sum = msg.cmd + msg.param1 + msg.param2 + msg.param3;
  result = (uint8_t)(sum & 0x000000FF);
  return result;
}


/* BRIEF: led control message handler
 * INPUT: led control message string
 * NOTE: must be no sapce in the input message.
 * return:
 *     true, if message format is correct and set led, 
 *     false, if message format is incorrect and do nothing to led.
 *
 * message string format: [switch,][hues,][saturation,][brightness]
   *  switch[0/1]
   *  hues[0-360]
   *  saturation[0-100]
   *  brightness[0-100]
   * eg (red):   1,  0,   100,  100
   *             |   |     |     |
   *            /    |     |     \
   *        switch hues saturation brightness
   * string length = 11 (no sapce)
  */
bool LedControlMsgHandler(unsigned char *Msg, unsigned int len)
{
  bool ret = false;
  float *color_t = NULL;
  unsigned char *ptr = NULL;
  unsigned char led_hues_str[4] = {0};
  unsigned char led_saturation_str[4] = {0};
  unsigned char led_brightness_str[4] = {0};
  int  led_hues_value = 0;
  int  led_saturation_value = 0;
  int  led_brightness_value = 0;
  
  char delims[] = ",";
  char *psubstr = NULL;
  
  //null msg
  if ((0 == len) || (NULL == Msg)){
    return false;
  }
  
  switch(Msg[0]) {
  case '0':{ // led off
    if(1 != len)
      ret = false;
    else{
      led_log("CloseLED.");
      CloseLED_RGB();
      ret = true;
    }
    break;
  }
  case '1':{  // led on
    if (len < 7) // strlen("1,0,0,0")
      ret = false;
    else{
      // get msg data
      ptr = (unsigned char*)malloc(len-1);
      memset(ptr, 0, len-1);
      memcpy(ptr, Msg+2, len-2);
      //led_log("ptr[%d]=%s", strlen((char *)ptr), ptr);
      
      // get HSB data string
      psubstr = strtok((char*)ptr, delims);
      if ((psubstr != NULL) && (strlen(psubstr)<=3)) {
        memcpy(led_hues_str, psubstr, strlen(psubstr)); 
        //led_log("led_hues_str[%d]=%s", strlen((char*)led_hues_str), led_hues_str);
        psubstr = strtok( NULL, delims );
        if ((psubstr != NULL) && (strlen(psubstr)<=3)) {
          memcpy(led_saturation_str, psubstr, strlen(psubstr)); 
          //led_log("led_saturation_str[%d]=%s", strlen((char*)led_saturation_str), led_saturation_str);
          psubstr = strtok( NULL, delims );
          if ((psubstr != NULL) && (strlen(psubstr)<=3)) {
            memcpy(led_brightness_str, psubstr, strlen(psubstr));
            //led_log("led_brightness_str[%d]=%s", strlen((char*)led_brightness_str), led_brightness_str);
            //get HSB string ok, transform to int and set LED
            if(Str2Int(led_hues_str, &led_hues_value) && \
              Str2Int(led_saturation_str, &led_saturation_value) \
                && Str2Int(led_brightness_str, &led_brightness_value)){
                  color_t = (float*)malloc(3);
                  if (NULL != color_t){
                    led_log("OpenLED: H=%d, S=%d, B=%d", 
                            led_hues_value, 
                            led_saturation_value, 
                            led_brightness_value);
                    H2R_HSBtoRGB((float)led_hues_value, (float)led_saturation_value, 
                                 (float)led_brightness_value, color_t);
                    OpenLED_RGB(color_t);
                    free(color_t);
                    color_t = NULL;
                    ret = true;
                  }
                }
          }
        }
      }
      if(NULL != ptr){
        free(ptr);
        ptr = NULL;
      }
    }
    break;
  }
  default:
    ret = false;
    break;
  }
  
  return ret;    
}

/*******************************************************************************
 * rewrite MVD Msg transform protocol API
 ******************************************************************************/

// rewrite this function by user
OSStatus MVDMsgTransformCloud2Device(unsigned char* inJsonString, unsigned int inJsonStringLen, 
                               unsigned char** outUsartMsg, unsigned int* outUsartMsgLen)
{
  led_log_trace();
  OSStatus err = kUnknownErr;
  
  bool ret = false;
  ret = LedControlMsgHandler(inJsonString, inJsonStringLen);
  
  if (ret){
    err = kNoErr;
  }
  else
  {
    err = kRequestErr;
  }
  
  return err;
  
  /*
  lm_usart_message_t usartMsg = {0};
  char cmdString[MAX_SIZE_CMD] = {0};
  json_object *new_obj = NULL;
  
  if(NULL == inJsonString || inJsonStringLen <= 0){
    return kParamErr;
  }
  
  // USART msg head && tail
  usartMsg.head = LED_USART_MSG_HEAD;
  usartMsg.tail = LED_USART_MSG_TAIL;
  
  //parse json data for USART msg cmd && params.
  new_obj = json_tokener_parse((char*)inJsonString);
  require_action(new_obj, exit, err = kUnknownErr);
  led_log("Recv json config object=%s", json_object_to_json_string(new_obj));
  
  json_object_object_foreach(new_obj, key, val) {
    if(!strcmp(key, "cmd")){
      memset(cmdString, 0, MAX_SIZE_CMD);
      strncpy(cmdString, json_object_get_string(val), MAX_SIZE_CMD);
      led_log("cmd=[%s]", cmdString);
    }
    else if( (!strcmp(key, LED_PARAM_WHITE)) || (!strcmp(key, LED_PARAM_HOUR)) ){
      usartMsg.param1 = (uint8_t)json_object_get_int(val);
      led_log("param1=%d", usartMsg.param1);
    }
    else if( (!strcmp(key, LED_PARAM_YELLOW)) || (!strcmp(key, LED_PARAM_MINUTE)) ){
      usartMsg.param2 = (uint8_t)json_object_get_int(val);
      led_log("param2=%d", usartMsg.param2);
    }
    else if( (!strcmp(key, LED_PARAM_BRIGHTNESS)) || (!strcmp(key, LED_PARAM_SECOND)) ){
      usartMsg.param3 = (uint8_t)json_object_get_int(val);
      led_log("param3=%d", usartMsg.param3);
    }
    else {
    }
  }          
  json_object_put(new_obj);
  
  // translate cmd value 
  switch(getCmdNum(cmdString, strlen(cmdString))){
  case 1:{ // on
    usartMsg.cmd = LED_CMD_ON;
    break;
  }
  case 2:{ // off
    usartMsg.cmd = LED_CMD_OFF;
    break;
  }
  case 3:{ // delay
    usartMsg.cmd = LED_CMD_DELAY;
     break;
  }
  case 4:{ // status
    usartMsg.cmd = LED_CMD_STATUS;
     break;
  }
  default:
    err = kUnknownErr;
    led_log("ERROR: unknown cmd!");
    break;
  }
  
  // USART msg checksum
  usartMsg.checksum = checksum(usartMsg);
  *outUsartMsg = (unsigned char*)malloc(sizeof(usartMsg));
  require( *outUsartMsg, exit );
  
  memset(*outUsartMsg, 0, sizeof(usartMsg));
  memcpy(*outUsartMsg, (char*)(&usartMsg), sizeof(usartMsg));
  *outUsartMsgLen = sizeof(usartMsg);
  err = kNoErr;
  
exit:
  return err; 
  */
}

// rewrite this interface by user
OSStatus MVDMsgTransformDevice2Cloud(unsigned char* inUsartString, unsigned int inUsartStringLen, 
                              unsigned char** outJson, unsigned int* outJsonLen)
{
  led_log_trace();
  OSStatus err = kUnknownErr;
  lm_usart_message_t usartMsg = {0};
  json_object *object = NULL;
  unsigned char *json_str = NULL;
  
  if(NULL == inUsartString || inUsartStringLen <= 0 
     || inUsartStringLen > sizeof(usartMsg)){
    return kParamErr;
  }
  
  memcpy((void*)&usartMsg, inUsartString, inUsartStringLen);
  //check response format
  if( (LED_USART_MSG_HEAD != usartMsg.head) ||
      (LED_USART_MSG_TAIL != usartMsg.tail) ||
      (checksum(usartMsg) != usartMsg.checksum) ){
    return kResponseErr;
  }
  
  // create json response to cloud
  object = json_object_new_object();
  require_action(object, exit, err = kNoMemoryErr);
  
  switch(usartMsg.cmd){
  case LED_RESP_SET_OK:{
    json_object_object_add(object, "response", 
                           json_object_new_string(LED_RESP_CMD_VALUE_OK)); 
    break;
  }
  case LED_RESP_STATUS:{
    json_object_object_add(object, "response", 
                           json_object_new_string(LED_RESP_CMD_VALUE_STATUS)); 
    json_object_object_add(object, LED_PARAM_WHITE, json_object_new_int(usartMsg.param1)); 
    json_object_object_add(object, LED_PARAM_YELLOW, json_object_new_int(usartMsg.param2));
    json_object_object_add(object, LED_PARAM_BRIGHTNESS, json_object_new_int(usartMsg.param3));
    break;
  }
  default:
    break;
  }
  
  //json string need to be free by user
  json_str = (unsigned char*)json_object_to_json_string(object);
  *outJsonLen = strlen((char*)(json_str));
  *outJson = (unsigned char*)malloc(*outJsonLen);
  if(NULL == (*outJson)){
    return kNoMemoryErr;
  }
  memcpy(*outJson, json_str, *outJsonLen);
  //led_log("outJson[%d]=%.*s", *outJsonLen, *outJsonLen, *outJson);
  return kNoErr;
  
exit:
  if(NULL != object){
    json_object_put(object);
    object = NULL;
  }
  return err;
}
