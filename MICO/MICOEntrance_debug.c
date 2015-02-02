#include <stdio.h>
#include <stdint.h>
#include "type.h"
#include "clk.h"
#include "uart.h"
#include "gpio.h"
#include "cache.h"
#include "delay.h"
//#include "app_config.h"
#include "debug.h"
#include "watchdog.h"
#include <stdio.h>

#define DBG printf

#define		BUART_RECVDAT_BYINT		//receive data in interrupt context

char BuartRxBuf[1024], BuartTxBuf[1024];
int BuartRxDatLen = 0,BuartRxDatIn = 0;
/**
 * @brief 	Buart interrupt service routine
 * @param	None
 * @return	None
 * @note
 */
//you can re-write this buart interrupt service routine by yourself
__attribute__((section(".driver.isr")))
void BuartInterrupt(void)
{
	long Status;
	/*
	 * take datum reception proirity over transmittion
	 */
	if((Status = BuartIOctl(UART_IOCTL_RXSTAT_GET,0)) & 0x1)
	{
		/*
		 * clear interrupt status firstly
		 */
        BuartIOctl(UART_IOCTL_RXINT_CLR,0);
#ifdef FREERTOS_VERSION /* OS message notification mechanism */
		/*
		 * notify the task that BUART data is ready & disable the BUART interrupt at the same time
		 */		
        BuartIOctl(UART_IOCTL_RXINT_SET,0);
		OSQueueMsgSend(MSGDEV_BUART_DATRDY, NULL, 0, MSGPRIO_LEVEL_MD, 0);
#elif	defined(BUART_RECVDAT_BYINT) /* receive datum  directly in interrupt */		
		//or you can receive data in interrupt context
		if(BuartRxDatLen < sizeof(BuartRxBuf))
		{
			int Echo;
			if(0 < (Echo = BuartRecv(&BuartRxBuf[BuartRxDatLen],sizeof(BuartRxBuf) - BuartRxDatLen,0)))			
				BuartRxDatLen += Echo;
		}

		else
			DBG("Buart Rx Buffer overflow\n");
#else	/*in interrupt set flag only,you can receive datum by polling when see this flag */
		BuartRxDatIn = 1;
#endif //FREERTOS_VERSION
	}

	if(Status & 0x1E)
	{
		/*
		 * tansfer error occurs if,give some hints,just be valid in non-btmode
		 */
#ifdef	DEBUG
		if(BuartIOctl(UART_IOCTL_RXSTAT_GET,0) & 0x2)
		{
			DBG(("Buart RX error interrupt\n"));
		}

		if(BuartIOctl(UART_IOCTL_RXSTAT_GET,0) & 0x4)
		{
			DBG(("Buart RX error overrun\n"));
		}

		if(BuartIOctl(UART_IOCTL_RXSTAT_GET,0) & 0x8)
		{
			DBG(("Buart RX error parity\n"));
		}

		if(BuartIOctl(UART_IOCTL_RXSTAT_GET,0) & 0x10)
		{
			DBG(("Buart RX error frame\n"));
		}
#endif	//DEBUG
		/*
		 * clear FIFO before clear other flags rudely
		 */
        BuartIOctl(UART_IOCTL_RXFIFO_CLR,0);
        BuartIOctl(BUART_IOCTL_RXFIFO_CLR,0);		
		/*
		 * clear other error flags
		 */        
		BuartIOctl(UART_IOCTL_RXINT_CLR,0);
	}
	if(BuartIOctl(UART_IOCTL_TXSTAT_GET,0) & 0x1)
	{
		/*
		 * clear interrupt status
		 */
        BuartIOctl(UART_IOCTL_TXINT_CLR,0);		
#ifdef FREERTOS_VERSION
		/*
		 * notify the task that BUART data is transferring done
		 */
		OSQueueMsgSend(MSGDEV_BUART_DATDON, NULL, 0, MSGPRIO_LEVEL_LO, 0);
#endif //FREERTOS_VERSION
	}
}


#if 0
static char Buf[0x100];
static int BufLen = 0;
//you can re-write Fuart interrupt service routine by yourself freely.
__attribute__((section(".driver.isr")))
void FuartInterrupt(void)
{
	int status;
	/*
	 * take reception proirity over transmittion
	 */
    status = FuartIOctl(UART_IOCTL_RXSTAT_GET,0);
	if(status & 0x1E)
	{
#ifdef	DEBUG
		/*
		 * tansfer error occurs if,give some hints
		 */
		if(status & 0x02)
		{
			DBG(("Fuart RX error interrupt\r\n"));
		}
		if(status & 0x04)
		{
			DBG(("Fuart RX error overrun\r\n"));
		}
		if(status & 0x08)
		{
			DBG(("Fuart RX error Parity\r\n"));
		}
		if(status & 0x10)
		{
			DBG(("Fuart RX error Frame\r\n"));
		}

#endif	//DEBUG
		/*
		 * clear FIFO before clear other flags
		 */
		FuartIOctl(UART_IOCTL_RXFIFO_CLR,0);
		/*
		 * clear other error flags
		 */
		FuartIOctl(UART_IOCTL_RXINT_CLR,0);
	}
	//else
	if(status & 0x01)
	{
#ifdef FREERTOS_VERSION
		/*
		 * notify the shell task to take preparation for datum coming
		 */
		OSQueueMsgSend(MSGDEV_FUART_DATRDY, NULL, 0, MSGPRIO_LEVEL_LO, 0);
#else        
		//or,you can receive them in the interrupt directly
		while(BufLen < 100)
			if(0 < FuartRecvByte(&Buf[BufLen]))
					BufLen ++;
			else
					break;
#endif //FREERTOS_VERSION
		FuartIOctl(UART_IOCTL_RXINT_CLR,0);
	}
	//else
	if(FuartIOctl(UART_IOCTL_TXSTAT_GET,0) & 0x01)
	{
		/*
		 * TO DO
		 */
		FuartIOctl(UART_IOCTL_TXINT_CLR,0);
	}
}
#endif //0



int main(void)
{
	int i,echo;
	int OutputAll = 0;
	
	
	ClkPorRcToDpll(0);
    CacheInit();
	ClkModuleEn(ALL_MODULE_CLK_SWITCH);
	ClkModuleGateEn(ALL_MODULE_CLK_GATE_SWITCH);	

    //SysTickInit();

	//Disable Watchdog
	WaitMs(200);
	WdgDis();

	GpioFuartRxIoConfig(1);
	GpioFuartTxIoConfig(1);

	FuartInit(115200,8,0,1);
	
	//RX GPIO B29
	GpioBuartRxIoConfig(3);
	//TX GPIO B28
	GpioBuartTxIoConfig(3);

	//enable RTS flow control,GPIO B31
	//GpioBuartRtsIoConfig(1);
	//BuartIOctl(BUART_IOCTL_RXRTS_FLOWCTL_SET, 2);
	//use the PMEM as the buart external FIFO,you can uncomment the following line to use internal 4 byte fifo
	BuartExFifoInit(0/*offset 0 from PMEM*/, 8192/*RX fifo 8KB*/, 2048/*TX fifo 2KB*/, 1/*interrupt trigger depth 1 byte*/);
	BuartInit(115200, 8, 0, 1);
	//enblae buart interrupt
	BuartIOctl(UART_IOCTL_RXINT_SET, 1);	
		
#ifdef FREERTOS_VERSION
	//register message set
	MsgAdd(MSGBUART_CLASS);
	BuartIOctl(UART_IOCTL_RXINT_SET,1);		
#elif	defined(BUART_RECVDAT_BYINT)
	BuartIOctl(UART_IOCTL_RXINT_SET,1);
	BuartRxDatLen = 0;				
#else
	BuartIOctl(UART_IOCTL_RXINT_SET,0);
	BuartRxDatIn = 0;	
#endif //FREERTOS_VERSION		
	DBG("Hello\n");
	//start to receive & send datum
	while(1)
	{
		//interrupt message notification
#ifdef FREERTOS_VERSION
		//register message set
		MsgAdd(MSGBUART_CLASS);
		MsgCode = OSQueueMsgRecv(NULL,NULL,-1);
		echo = 0;
		switch(MsgCode)
		{
		case 	MSGBUART_DATRDY:
				//data is incoming
				echo = BuartRecv(BuartRxBuf,1024,0);
				BuartIOctl(UART_IOCTL_RXINT_SET,1);
				break;
		
		default:
				break;
		}
		for(i = 0;i < echo && i < 1024;i ++)
		{
			if(0 == OutputAll % 32)
				DBG("\n");
			DBG("%02x, ",BuartRxBuf[i]);
			OutputAll ++;
		}
		//interrupt service function receive
#elif	defined(BUART_RECVDAT_BYINT)
		echo = BuartRxDatLen;
		for(i = 0;i < echo && i < 1024;i ++)
		{
			if(0 == OutputAll % 32)
				DBG("\n");
			DBG("%02x, ",BuartRxBuf[i]);
			OutputAll ++;
		}			
#else
		//polling receive
		if(BuartRxDatIn)
		{
			//read out the FIFO data till FIFO empty
			while(0 < BuartIOctl(BUART_IOCTL_RXFIFO_DATLEN_GET,0))				
			{
				int i,echo;
				echo = BuartRecv(BuartRxBuf,1024,0);
				for(i = 0;i < echo && i < 1024;i ++)
				{
					if(0 == OutputAll % 32)
						DBG("\n");
					DBG("%02x, ",BuartRxBuf[i]);
					OutputAll ++;
				}
				if(0 == OutputAll % 100)
					BuartSend("receive 100 bytes\n",sizeof("receive 100 bytes\n") - 1);
			}
			//reset flag;
			BuartRxDatIn = 0;
		}		
#endif //FREERTOS_VERSION					
		
		//will be wake up by Buart interrupt
 		__wfi();
		DBG("wakeup by buart datum interrupt\n");
	}
}

