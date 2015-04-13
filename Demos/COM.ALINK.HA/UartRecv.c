
#include "MICODefine.h"
#include "MICOAppDefine.h"

#include "MicoPlatform.h"
#include "platform.h"
#include "MICONotificationCenter.h"
#include "device.h"

#define uart_recv_log(M, ...) custom_log("UART RECV", M, ##__VA_ARGS__)
#define uart_recv_log_trace() custom_log_trace("UART RECV")

static int _uart_get_one_packet(uint8_t* buf, int maxlen);

void uartRecv_thread(void *inContext)
{
  uart_recv_log_trace();
  int recvlen;
  uint8_t *inDataBuffer;
  
  inDataBuffer = malloc(UartRecvBufferLen);
  require(inDataBuffer, exit);
  
  while(1) {
    recvlen = _uart_get_one_packet(inDataBuffer, UartRecvBufferLen);
    if (recvlen <= 0)
      continue; 
    device_cmd_process(inDataBuffer, recvlen);
  }
  
exit:
  if(inDataBuffer) free(inDataBuffer);
}

/* Packet format: BB 00 CMD(2B) Status(2B) datalen(2B) data(x) checksum(2B)
* copy to buf, return len = datalen+10
*/
int _uart_get_one_packet(uint8_t* inBuf, int inBufLen)
{
  uart_recv_log_trace();
  OSStatus err = kNoErr;
  int datalen;
  uint8_t *p;

  while(1) {
    p = inBuf;
    err = MicoUartRecv(UART_FOR_APP, p, 1, MICO_WAIT_FOREVER);
    require_noerr(err, exit);
    require(*p == 0x3A, exit);
    p++;
    
    err = MicoUartRecv(UART_FOR_APP, p, 5, 500);
    require_noerr(err, exit);
    datalen = p[1];
    require(datalen + 10 <= inBufLen, exit);
    require(datalen >= 6, exit);
    p += 5;
    
    err = MicoUartRecv(UART_FOR_APP, p, datalen-6, 500);
    require_noerr(err, exit);
    
//    err = check_sum(inBuf, datalen + 10);
//    require_noerr(err, exit);
    return datalen;
  }
exit:
  return -1;
  
}


