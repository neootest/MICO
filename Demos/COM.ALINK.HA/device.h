#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "json.h"
#include "MICODefine.h"
#include "alink_export.h"

typedef struct
{
  int idx;
  const char *name;
} device_func_index;

typedef struct _uart_cmd {
  uint8_t power; 
  uint8_t tt;
  uint8_t ct;
  uint8_t heatmode;
  uint8_t dspheatmode;
  uint8_t max;
  uint8_t onoffheat;
  uint8_t aesflag; 
  uint8_t onofftimerpeak; 
  uint8_t onofftimer1; 
  uint8_t onofftimer2;
  uint8_t peaktimestart;
  uint8_t peaktimestop;
  uint8_t time1start; 
  uint8_t time1stop;
  uint8_t time2start; 
  uint8_t time2stop;
  const char *error;
}uart_cmd_t;

#pragma pack(push)
#pragma pack(1)
typedef struct _device_cmd_head {
  uint16_t flag;
  uint8_t datalen;
  uint16_t protocol_ver;
  uint8_t command;
  uint8_t data[1];
}device_cmd_head_t;
#pragma pack(pop)

enum{
  power = 1,
  tt,
  ct,
  heatMode,
  dspHeatmode,
  max,
  OnOff_Heat,
  AESFlag,
  OnOff_TimerPeak,
  OnOff_Timer1,
  OnOff_Timer2,
  peaktimestart,
  peaktimestop,
  ontimestart1,
  ontimestop1,
  ontimestart2,
  ontimestop2,
  errCode
};

typedef struct _device_func{
  uint8_t func;
  uint32_t value;
}device_func_t;

typedef struct _device_func_json{
  const char *value;
}device_func_json_t;

typedef struct _device_func_json_extra{
  const char *value;
  const char *extra;
}device_func_json_extra_t;

typedef struct _device_status{
  device_func_json_t heatermode;
  device_func_json_t ontime;
  device_func_json_t power;
  device_func_json_t currtemp;
  device_func_json_t temp;
  device_func_json_t watervol;
  device_func_json_t powersave;
  device_func_json_t heating;
  device_func_json_t switch1;
  device_func_json_t erro;
}device_status_t;

int device_command_execute(alink_down_cmd_ptr down_cmd);
int device_status_post(alink_down_cmd_ptr down_cmd);
void device_cmd_process(uint8_t *buf, int inLen);
#endif