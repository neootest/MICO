#ifndef __MICO_APP_DEFINE_H
#define __MICO_APP_DEFINE_H

#include "Common.h"
#include "Debug.h"

#define APP_INFO   "mxchipWNet HA Demo based on MICO OS"

#define FIRMWARE_REVISION   "MICO_HA_4_0"
#define MANUFACTURER        "MXCHIP Inc."
#define SERIAL_NUMBER       "20140606"
#define PROTOCOL            "com.mxchip.ha"
#define LOCAL_PORT          8080

/* Wi-Fi configuration mode */
#define MICO_CONFIG_MODE CONFIG_MODE_EASYLINK_WITH_SOFTAP

/*User provided configurations*/
#define CONFIGURATION_VERSION         0x0000031 // if changed default configuration, add this num
#define MAX_Local_Client_Num          8
#define DEAFULT_REMOTE_SERVER         "192.168.2.254"
#define DEFAULT_REMOTE_SERVER_PORT    8080
#define UART_BUFFER_LENGTH            2048

#define BONJOUR_SERVICE                     "_easylink._tcp.local."

#define LOCAL_TCP_SERVER_LOOPBACK_PORT     1000
#define REMOTE_TCP_CLIENT_LOOPBACK_PORT    1002
#define RECVED_UART_DATA_LOOPBACK_PORT     1003

/*Application's configuration stores in flash*/
typedef struct
{
  uint32_t          configDataVer;
  uint32_t          localServerPort;

  /*local services*/
  bool              localServerEnable;
  bool              remoteServerEnable;
  char              remoteServerDomain[64];
  int               remoteServerPort;

  /*IO settings*/
  uint32_t          USART_BaudRate;
} application_config_t;


#define wlanBufferLen       1024
#define UartRecvBufferLen   1024

/*Running status*/
typedef struct _current_app_status_t {
  /*Local clients port list*/
  uint32_t          loopBack_PortList[MAX_Local_Client_Num];
} current_app_status_t;


void localTcpServer_thread(void *inContext);
void remoteTcpClient_thread(void *inContext);
void uartRecv_thread(void *inContext);



#endif

