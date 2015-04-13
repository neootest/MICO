
#include "MICOAppDefine.h"
#include "HAProtocol.h"
#include "SocketUtils.h"
#include "debug.h"
#include "MicoPlatform.h"
#include "platform_config.h"
#include "MICONotificationCenter.h"
#include <stdio.h>

#define ha_log(M, ...) custom_log("HA Command", M, ##__VA_ARGS__)
#define ha_log_trace() custom_log_trace("HA Command")

//static u32 running_state = 0;
static uint32_t network_state = 0;
static mico_mutex_t _mutex;

bool global_wifi_status = false;
mico_semaphore_t uuid_sem;

void haNotify_WifiStatusHandler(int event, mico_Context_t * const inContext)
{
  ha_log_trace();
  (void)inContext;
  switch (event) {
  case NOTIFY_STATION_UP:
    set_network_state(STA_CONNECT, 1);
    global_wifi_status = true;
    break;
  case NOTIFY_STATION_DOWN:
    set_network_state(STA_CONNECT, 0);
    global_wifi_status = false;
    break;
  default:
    break;
  }
  return;
}


int is_network_state(int state)
{
  if ((network_state & state) == 0)
    return 0;
  else
    return 1;
}



void set_network_state(int state, int on)
{
  ha_log_trace();
  mico_rtos_lock_mutex(&_mutex);
  if (on)
    network_state |= state;
  else {
    network_state &= ~state;
    if (state == STA_CONNECT)
      network_state &= ~REMOTE_CONNECT;
  }
  
  mico_rtos_unlock_mutex(&_mutex);
}


OSStatus haProtocolInit(mico_Context_t * const inContext)
{
  ha_log_trace();
  OSStatus err = kUnknownErr;

  mico_rtos_init_mutex(&_mutex);
  mico_rtos_init_semaphore(&uuid_sem, 1);

  /* Regisist notifications */
  err = MICOAddNotification( mico_notify_WIFI_STATUS_CHANGED, (void *)haNotify_WifiStatusHandler );
  require_noerr( err, exit ); 
exit:
  return err;
}




