#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
#include "platform.h"
#include "MICO.h"
#include "EasyLink.h"
#include "MICONotificationCenter.h"

#include "StringUtils.h"
#include "HTTPUtils.h"
#include "SocketUtils.h"
#include "device.h"
#include "stringj.h"

#include "json.h"

#define device_log(M, ...) custom_log("DEVICE", M, ##__VA_ARGS__)
#define device_log_trace() custom_log_trace("DEVICE")
#define DAY_SECONDS (24*60*60)
#define UART_BUFFER_LEN 200

#define FILL(a)   {a, #a}

void parseDeviceStatus(uint8_t *inBuf, uint8_t send);
extern void PlatformEasyLinkButtonClickedCallback(void);

static uart_cmd_t uartcmd;
static int hour,min;
static char account[20] = {0};
static alink_down_cmd g_down_cmd = {-1, NULL, account, NULL, NULL, NULL};
static char error_code[30] = {0};

device_func_index hiFuncStr[21] = {
  FILL(power),
  FILL(heatMode),
  FILL(dspHeatmode),
  FILL(ct),
  FILL(tt),
  FILL(max),
  FILL(OnOff_Heat),
  FILL(AESFlag),
  FILL(OnOff_TimerPeak),
  FILL(OnOff_Timer1),
  FILL(OnOff_Timer2),
  FILL(peaktimestart),
  FILL(peaktimestop),
  FILL(ontimestart1),
  FILL(ontimestop1),
  FILL(ontimestart2),
  FILL(errCode),
  FILL(ontimestop2),
  {0, NULL}
};

static uint8_t out_buffer[UART_BUFFER_LEN];

const char dtrCrcTable[256] = {
  0x00, 0x9b, 0xad, 0x36, 0xc1, 0x5a, 0x6c, 0xf7,
  0x19, 0x82, 0xb4, 0x2f, 0xd8, 0x43, 0x75, 0xee,
  0x32, 0xa9, 0x9f, 0x04, 0xf3, 0x68, 0x5e, 0xc5,
  0x2b, 0xb0, 0x86, 0x1d, 0xea, 0x71, 0x47, 0xdc,
  0x64, 0xff, 0xc9, 0x52, 0xa5, 0x3e, 0x08, 0x93,
  0x7d, 0xe6, 0xd0, 0x4b, 0xbc, 0x27, 0x11, 0x8a,
  0x56, 0xcd, 0xfb, 0x60, 0x97, 0x0c, 0x3a, 0xa1,
  0x4f, 0xd4, 0xe2, 0x79, 0x8e, 0x15, 0x23, 0xb8,
  0xc8, 0x53, 0x65, 0xfe, 0x09, 0x92, 0xa4, 0x3f,
  0xd1, 0x4a, 0x7c, 0xe7, 0x10, 0x8b, 0xbd, 0x26,
  0xfa, 0x61, 0x57, 0xcc, 0x3b, 0xa0, 0x96, 0x0d,
  0xe3, 0x78, 0x4e, 0xd5, 0x22, 0xb9, 0x8f, 0x14,
  0xac, 0x37, 0x01, 0x9a, 0x6d, 0xf6, 0xc0, 0x5b,
  0xb5, 0x2e, 0x18, 0x83, 0x74, 0xef, 0xd9, 0x42,
  0x9e, 0x05, 0x33, 0xa8, 0x5f, 0xc4, 0xf2, 0x69,
  0x87, 0x1c, 0x2a, 0xb1, 0x46, 0xdd, 0xeb, 0x70,
  0x0b, 0x90, 0xa6, 0x3d, 0xca, 0x51, 0x67, 0xfc,
  0x12, 0x89, 0xbf, 0x24, 0xd3, 0x48, 0x7e, 0xe5,
  0x39, 0xa2, 0x94, 0x0f, 0xf8, 0x63, 0x55, 0xce,
  0x20, 0xbb, 0x8d, 0x16, 0xe1, 0x7a, 0x4c, 0xd7,
  0x6f, 0xf4, 0xc2, 0x59, 0xae, 0x35, 0x03, 0x98,
  0x76, 0xed, 0xdb, 0x40, 0xb7, 0x2c, 0x1a, 0x81,
  0x5d, 0xc6, 0xf0, 0x6b, 0x9c, 0x07, 0x31, 0xaa,
  0x44, 0xdf, 0xe9, 0x72, 0x85, 0x1e, 0x28, 0xb3,
  0xc3, 0x58, 0x6e, 0xf5, 0x02, 0x99, 0xaf, 0x34,
  0xda, 0x41, 0x77, 0xec, 0x1b, 0x80, 0xb6, 0x2d,
  0xf1, 0x6a, 0x5c, 0xc7, 0x30, 0xab, 0x9d, 0x06,
  0xe8, 0x73, 0x45, 0xde, 0x29, 0xb2, 0x84, 0x1f,
  0xa7, 0x3c, 0x0a, 0x91, 0x66, 0xfd, 0xcb, 0x50,
  0xbe, 0x25, 0x13, 0x88, 0x7f, 0xe4, 0xd2, 0x49,
  0x95, 0x0e, 0x38, 0xa3, 0x54, 0xcf, 0xf9, 0x62,
  0x8c, 0x17, 0x21, 0xba, 0x4d, 0xd6, 0xe0, 0x7b,
};

static uint8_t calc_sum(unsigned char *rCRCDataBuf, uint32_t len)
{
  uint8_t rCRCResult = 0;
  int temp;
  
  do {
    temp = *rCRCDataBuf^rCRCResult;
    rCRCResult = dtrCrcTable[temp];
    rCRCDataBuf++;
  } while (--len);
  return( rCRCResult );
}

OSStatus sendCommandToDevice(int len)
{
  return MicoUartSend(UART_FOR_APP, out_buffer, len);
}

void commandToDevice(device_func_t command)
{
  device_log_trace();
  device_cmd_head_t *p_out;
  
  p_out = (device_cmd_head_t *)out_buffer;
  p_out->flag = 0xea3a;
  p_out->protocol_ver = 0;
  
  switch(command.func){
  case power:
    p_out->command = 1;
    p_out->data[0] = command.value;
    p_out->datalen = 8;
    break;
  case tt:
    p_out->command = 2;
    p_out->data[0] = command.value;
    p_out->datalen = 8;
    break;
  case heatMode:
    p_out->command = 3;
    p_out->data[0] = command.value;
    p_out->datalen = 8;
    break;
  case max:
    p_out->command = 4;
    p_out->data[0] = command.value;
    p_out->datalen = 8;
    break;
  case OnOff_Heat:
    p_out->command = 0x0D;
    p_out->data[0] = command.value;
    p_out->datalen = 8;
    break;
  case AESFlag:
    p_out->command = 5;
    p_out->data[0] = command.value;
    p_out->datalen = 8;
    break;
  case OnOff_TimerPeak:
    p_out->command = 6;
    p_out->data[0] = command.value;
    p_out->data[1] = uartcmd.peaktimestart/60;
    p_out->data[2] = uartcmd.peaktimestart%60;
    p_out->data[3] = uartcmd.peaktimestop/60;
    p_out->data[4] = uartcmd.peaktimestop%60;
    p_out->datalen = 12;
    break;
  case OnOff_Timer1:
    p_out->command = 7;
    p_out->data[0] = command.value;
    p_out->data[1] = uartcmd.time1start/60;
    p_out->data[2] = uartcmd.time1start%60;
    p_out->data[3] = uartcmd.time1stop/60;
    p_out->data[4] = uartcmd.time1stop%60;
    p_out->datalen = 12;
    break;
  case OnOff_Timer2:
    p_out->command = 8;
    p_out->data[0] = command.value;
    p_out->data[1] = uartcmd.time2start/60;
    p_out->data[2] = uartcmd.time2start%60;
    p_out->data[3] = uartcmd.time2stop/60;
    p_out->data[4] = uartcmd.time2stop%60;
    p_out->datalen = 12;
    break;
  case peaktimestart:
    p_out->command = 6;
    p_out->data[0] = 1;
    p_out->data[1] = hour;
    p_out->data[2] = min;
    p_out->datalen = 12;
    break;
  case peaktimestop:
    p_out->command = 6;
    p_out->data[0] = 1;
    p_out->data[3] = hour;
    p_out->data[4] = min;
    p_out->datalen = 12;
    break;
  case ontimestart1:
    p_out->command = 7;
    p_out->data[0] = 1;
    p_out->data[1] = hour;
    p_out->data[2] = min;
    p_out->datalen = 12;
    break;
  case ontimestop1:
    p_out->command = 7;
    p_out->data[0] = 1;
    p_out->data[3] = hour;
    p_out->data[4] = min;
    p_out->datalen = 12;
    break;
  case ontimestart2:
    p_out->command = 8;
    p_out->data[0] = 1;
    p_out->data[1] = hour;
    p_out->data[2] = min;
    p_out->datalen = 12;
    break;
  case ontimestop2:
    p_out->command = 8;
    p_out->data[0] = 1;
    p_out->data[3] = hour;
    p_out->data[4] = min;
    p_out->datalen = 12;
    break;
  }
  p_out->data[p_out->datalen-7] = calc_sum(out_buffer, p_out->datalen-1);
}

static void parseUTCtime(device_func_t command, const char *quartz)
{
  if(NULL == quartz)
    return;
  sscanf(quartz, "UTC+08:00 0 %d %d", &min, &hour);
}

int device_command_execute(alink_down_cmd_ptr down_cmd)
{
  int i;
  json_value *jsonObj, *j_param, *j_ex;
  json_value *j_val;
  char *valueStr, *exStr;
  device_func_t cmd;
  device_log("alink_sample_set_device_status called:------------\n%s %d\n%s\n", down_cmd->uuid, down_cmd->method, down_cmd->param);
  jsonObj = json_parse(down_cmd->param, strlen(down_cmd->param));
  
  memset(out_buffer, 0, UART_BUFFER_LEN);
  
  for (i = 0; hiFuncStr[i].name != NULL; i++)
  {
    j_param = json_object_object_get_e(jsonObj, (char *)hiFuncStr[i].name);
    if (j_param != NULL)
    {
      j_val = json_object_object_get_e(j_param, "value");      
      valueStr = json_object_to_json_string_e(j_val);      
      //      alink_string_to_standard(valueStr);
      cmd.func = hiFuncStr[i].idx;
      if(cmd.func==ontimestart1||cmd.func==ontimestop1||cmd.func==ontimestart2||cmd.func==ontimestop2||cmd.func==peaktimestart||cmd.func==peaktimestop){
        j_ex = json_object_object_get_e(j_param, "extra");
        exStr = json_object_to_json_string_e(j_ex);
        //        alink_string_to_standard(exStr);
        parseUTCtime(cmd, exStr);
        commandToDevice(cmd);
      }
      else
      {        
        cmd.value = atoi(valueStr);
        commandToDevice(cmd);
      }
    }
  }
  json_value_free(jsonObj);
  //need to report devicd status after command has excuted
  g_down_cmd.id = down_cmd->id;
  g_down_cmd.account = account;
  strncpy(g_down_cmd.account, down_cmd->account, sizeof(account));
  
  return sendCommandToDevice(out_buffer[2]);
}

void fillDeviceStatus(char *str)
{
  device_log_trace();
  mico_Context_t *context = getGlobalContext();
  
  sprintf(str, post_str_format, context->flashContentInRam.appConfig.uuid, uartcmd.power, uartcmd.tt, uartcmd.ct,
          uartcmd.heatmode, uartcmd.dspheatmode, uartcmd.max, uartcmd.onoffheat, uartcmd.aesflag,
          uartcmd.onofftimerpeak, uartcmd.onofftimer1, uartcmd.onofftimer2, uartcmd.onofftimerpeak,
          uartcmd.peaktimestart%60, uartcmd.peaktimestart/60, uartcmd.onofftimerpeak,
          uartcmd.peaktimestop%60, uartcmd.peaktimestop/60, uartcmd.onofftimer1,
          uartcmd.time1start%60, uartcmd.time1start/60, uartcmd.onofftimer1,
          uartcmd.time1stop%60, uartcmd.time1stop/60,uartcmd.onofftimer2,
          uartcmd.time2start%60, uartcmd.time2start/60, uartcmd.onofftimer2,
          uartcmd.time2stop%60, uartcmd.time2stop/60, error_code);
}

static char _buf[1500];
// alink callback
int device_status_post(alink_down_cmd_ptr down_cmd) 
{
  int ret_code = 0;
  alink_up_cmd up_cmd;
  //  custom_log("alink_sample_get_device_status called:------------\n%s %d\n%s\n", down_cmd->uuid, down_cmd->method, down_cmd->param);    
  up_cmd.resp_id = down_cmd->id;
  fillDeviceStatus(_buf);
  up_cmd.param = _buf;
  up_cmd.target = down_cmd->account;
  
  ret_code = alink_post_device_data_array(&up_cmd);
  memset(account, 0, sizeof(account));
  g_down_cmd.id = -1;
  return ret_code;
}

enum {
  WATER_CALLBACK = 0xFC,
  SOFTAP_CALLBACK = 0XF7,
  CONTROL_EASYLINK = 0xFD, 
  ASK_VERSION = 0xF5,
};

static void send_aos_cmd(uint8_t type)
{
  uint8_t buf[8];
  device_cmd_head_t *cmd_header = (device_cmd_head_t *)buf;
  
  memset(buf, 0, sizeof(buf));
  cmd_header->flag=0xe13a;
  cmd_header->command = type;
  cmd_header->datalen = 8;
  cmd_header->data[0] = 0;
  cmd_header->data[1] = calc_sum((uint8_t *)&cmd_header, 7);
  MicoUartSend(UART_FOR_APP, buf, 8);
}

//static uint8_t heat_status = 0;
void device_cmd_process(uint8_t *buf, int inLen)
{
  device_log_trace();
  
  if (0x3A != buf[0])
    return;
  if(buf[1] == 0xEB){
    uartcmd.ct = buf[10];
    uartcmd.tt = buf[11];
    uartcmd.heatmode = buf[12]; 
    uartcmd.power = buf[13]&1;
    uartcmd.max = (buf[13]&2)>>1;
    uartcmd.aesflag = (buf[13]&4)>>2;
    uartcmd.onoffheat = buf[14]&1;
    //    uartcmd.maintain_remind = (buf[41]&0x10)>>4;
    //    debug_out("maintain_remind = %d", uartcmd.maintain_remind);
    //    uartcmd.set_appointment = (buf[13]&0x40)>>5;
    uartcmd.onofftimerpeak = (buf[13]&8)>>3;
    uartcmd.onofftimer1 = (buf[13]&0x10)>>4;
    uartcmd.onofftimer2 = (buf[13]&0x20)>>5;
    //    heat_status = uartcmd.status;
    uartcmd.dspheatmode = (buf[13]&0x80)>>7;
    //    if(heat_status!=uartcmd.status)
    //      sleep(1);
    
    uartcmd.peaktimestart = buf[17]*60 + buf[18];
    uartcmd.peaktimestop = buf[19]*60 + buf[20];
    uartcmd.time1start = buf[21]*60 + buf[22];
    uartcmd.time1stop = buf[23]*60 + buf[24];
    uartcmd.time2start = buf[25]*60 + buf[26];
    uartcmd.time2stop = buf[27]*60 + buf[28];
    
    memset(error_code, 0, 30);
    if(buf[40]&1)
      sprintf(error_code, "1");
    if(buf[40]&2){
      if(error_code[0]==0)
        sprintf(error_code, "2");
      else
        sprintf(error_code, "%s,%s", error_code, "2");
    }
    if(buf[40]&4){
      if(error_code[0]==0)
        sprintf(error_code, "3");
      else
        sprintf(error_code, "%s,%s", error_code, "3"); 
    }
    if(buf[40]&8){
      if(error_code[0]==0)
        sprintf(error_code, "5");
      else
        sprintf(error_code, "%s,%s", error_code, "5");
    }
    if(buf[40]&0x10){
      if(error_code[0]==0)
        sprintf(error_code, "4");
      else
        sprintf(error_code, "%s,%s", error_code, "4");
    } 
    if(buf[41]&1){
      if(error_code[0]==0)
        sprintf(error_code, "6");
      else
        sprintf(error_code, "%s,%s", error_code, "6");
    }
    if(error_code[0]==0) 
      sprintf(error_code, "0");    
    
    uartcmd.error = error_code;
  }else if(buf[1] == 0xE0){
    switch(buf[5]) {
    case CONTROL_EASYLINK:
      send_aos_cmd(0xff);
      
      PlatformEasyLinkButtonClickedCallback();
      break;
      //    case TEST_MODE:  //3A AA 08 AA AA AA AA AA
      //      debug_count = 2;
      //      test_mode();
      //      break;
    case WATER_CALLBACK:    
      break;
    case SOFTAP_CALLBACK:
      break;
    case 0xfa:
      msleep(10);
      //    hal_uart_send_data((u8 *)cmd_header,8);
      break; 
    case 0xf6:
      send_aos_cmd(ASK_VERSION);
      msleep(50);
      send_aos_cmd(ASK_VERSION);
      msleep(50);
      send_aos_cmd(ASK_VERSION);
      break;
    default:
      break;
    }
  }else
    return;
  device_status_post(&g_down_cmd);
}
