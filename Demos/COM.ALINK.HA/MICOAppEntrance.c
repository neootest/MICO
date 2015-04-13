#include "MICO.h"
#include "MICODefine.h"
#include "MICOAppDefine.h"

#include "StringUtils.h"
#include "SocketUtils.h"

#include "platform.h"
#include "MicoPlatform.h"

#include "haProtocol.h"


#define app_log(M, ...) custom_log("APP", M, ##__VA_ARGS__)
#define app_log_trace() custom_log_trace("APP")

volatile ring_buffer_t  rx_buffer;
volatile uint8_t        rx_data[UART_BUFFER_LENGTH];

extern bool             global_wifi_status;
mico_semaphore_t        ota_sem;
extern json_object* ConfigCreateReportJsonMessage( mico_Context_t * const inContext );
extern void ota_thread(void *inContext);

//static char hugebuf[128];
mico_mutex_t printf_mutex;

#define UDP_BROADCAST_PORT 34567
#if 0
static void udpSearch_thread(void *inContext)
{
  int len;
  fd_set readfds;
  struct timeval_t t;
  struct sockaddr_t addr;
  socklen_t addrLen;
  //  Context = inContext;
  json_object* report = NULL;
  const char *  json_str;
  char jSon_report[1024];
  
  udpSearch_fd = socket(AF_INET, SOCK_DGRM, IPPROTO_UDP);
  addr.s_ip = INADDR_ANY;
  addr.s_port = UDP_BROADCAST_PORT;
  bind(udpSearch_fd, &addr, sizeof(addr));
  
  report = ConfigCreateReportJsonMessage( inContext );
  json_str = json_object_to_json_string(report);
  memset(jSon_report, 0x00, 1024);
  memcpy(jSon_report, json_str, strlen(json_str));
  json_object_put(report);
  
  while(1) {
    FD_ZERO(&readfds);
    t.tv_sec = 10;
    t.tv_usec = 0;
    
    FD_SET(udpSearch_fd, &readfds);
    
    select(1, &readfds, NULL, NULL, &t);
    
    if (FD_ISSET(udpSearch_fd, &readfds)) {
      len = recvfrom(udpSearch_fd, hugebuf, 128, 0, &addr, &addrLen);
      if(!strcmp((char *)hugebuf,"query device mac")){
        sendto(udpSearch_fd, jSon_report, strlen(jSon_report), \
          0, &addr, sizeof(addr));
      }
      //search_cmd_process(hugebuf, len, inContext, &addr, sizeof(struct sockaddr_t));
    }
  }
  //mico_rtos_delete_thread(NULL);
}
#endif
/* MICO system callback: Restore default configuration provided by application */
void appRestoreDefault_callback(mico_Context_t *inContext)
{
  inContext->flashContentInRam.appConfig.configDataVer = CONFIGURATION_VERSION;
  inContext->flashContentInRam.appConfig.resetflag = 0xFF;
  memset(inContext->flashContentInRam.appConfig.uuid, 0xFF, sizeof(inContext->flashContentInRam.appConfig.uuid));
}

static void rpc_thread(void *inContext)
{
  mico_Context_t* Context = inContext;
  extern mico_semaphore_t      uuid_sem;
  const char *uuid;

  while(global_wifi_status == false) {
    msleep(100);
  }
   // app_log("Memory remains %d", micoGetMemoryInfo()->free_memory);
  if(Context->flashContentInRam.micoSystemConfig.configured == allConfigured){
    mico_rtos_init_semaphore(&ota_sem, 1);
    mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "ota", ota_thread,
                            0x700, inContext);
    mico_rtos_get_semaphore(&ota_sem, 30000);
  }

  mico_start_alink();
  while(ALINK_ERR == alink_wait_connect(NULL, 10000)){
    sleep(10);
  }
  uuid = alink_get_uuid(NULL);
  app_log("Main device UUID: %s", uuid);
  mico_rtos_set_semaphore(&uuid_sem);
  mico_rtos_delete_thread(NULL);
}

OSStatus start_rpc(void *inContext)
{
  app_log_trace();
  
  OSStatus err = kNoErr;

  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Rpc Server", rpc_thread, 0x800, (void *)inContext);
  require_noerr_action(err, exit, app_log("ERROR: Unable to start the rpc server thread."));
  
exit:
  return err;
}

OSStatus MICOStartApplication( mico_Context_t * const inContext )
{
  app_log_trace();

  OSStatus err = kNoErr;
  mico_uart_config_t uart_config;
  require_action(inContext, exit, err = kParamErr);

  haProtocolInit(inContext);
  mico_rtos_init_mutex(&printf_mutex);
  /*Bonjour for service searching*/
  //if(inContext->flashContentInRam.micoSystemConfig.bonjourEnable == true)
  //  MICOStartBonjourService( Station, inContext );
  
  /*UART receive thread*/
  uart_config.baud_rate    = UART_DEFAULT_BAUDRATE;
  uart_config.data_width   = DATA_WIDTH_8BIT;
  uart_config.parity       = NO_PARITY;
  uart_config.stop_bits    = STOP_BITS_1;
  uart_config.flow_control = FLOW_CONTROL_DISABLED;
  if(inContext->flashContentInRam.micoSystemConfig.mcuPowerSaveEnable == true)
    uart_config.flags = UART_WAKEUP_ENABLE;
  else
    uart_config.flags = UART_WAKEUP_DISABLE;
  ring_buffer_init  ( (ring_buffer_t *)&rx_buffer, (uint8_t *)rx_data, UART_BUFFER_LENGTH );
  MicoUartInitialize( UART_FOR_APP, &uart_config, (ring_buffer_t *)&rx_buffer );
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART Recv", uartRecv_thread, 0x800, (void*)inContext );
  require_noerr_action( err, exit, app_log("ERROR: Unable to start the uart recv thread.") );

  start_rpc(inContext);
exit:
  return err;
}




