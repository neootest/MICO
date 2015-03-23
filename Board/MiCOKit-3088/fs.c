#include "type.h"
#include "app_config.h"
#include "uart.h"
#include "key.h"
#include "clk.h"
#include "spi_flash.h"
#include "cache.h"
#include "gpio.h"
#include "dac.h"
#include "audio_adc.h"
#include "wakeup.h"
#include "timer.h"
#include "sys_app.h"
#include "rtc.h"
#include "adc.h"
#include "host_hcd.h"
#include "watchdog.h"
#include "mixer.h"
#include "breakpoint.h"
#include "dev_detect_driver.h"
#include "sys_vol.h"
#include "eq.h"
#include "lcd_seg.h"
#include "eq_params.h"
#include "sound_remind.h"
#include "sw_uart.h"
#include "debug.h"
#include "fat_file.h" 
#include "sd_card.h"
#include "host_stor.h"
#include "dir.h"

static FAT_FILE  TestFile;
static FOLDER	 RootFolder;
static FOLDER	 TestFolder;

uint16_t	LongName[FAT_NAME_MAX/2];
uint8_t	FolderShortName[11] = {'1', '2', '3', '4', '5', '6', '~', '1', ' ', ' ', ' '};

uint8_t Buf[50] = "test string";
uint8_t Buf2[50];
uint32_t WriteLen, ReadLen;
extern void SysTickInit(void);
extern int utf8_to_utf16(uint16_t* output, const uint8_t* input, uint32_t outsize, uint32_t insize);
	#define CARD_DETECT_PORT_IN			GPIO_A_IN	
	#define CARD_DETECT_PORT_OE			GPIO_A_OE	
	#define CARD_DETECT_PORT_PU			GPIO_A_PU	
	#define CARD_DETECT_PORT_PD			GPIO_A_PD	
	#define CARD_DETECT_PORT_IE  		GPIO_A_IE
	#define CARD_DETECT_BIT_MASK		(1 << 20)
	
uint8_t UartRec;
#define 	SetKeyValue(x)  (UartRec = (x)) 
#define 	GetKeyValue() 	(UartRec)
#define 	ClrKeyValue() 	(UartRec = 0) 
bool CheckAllDiskLinkFlag(void)
{
    return TRUE;
}
static __INLINE void CardDetectDelay(void)
{
	__nop();
	__nop();
	__nop();
	__nop();
	__nop();
}

bool IsCardLink(void)
{
	bool TempFlag;
	LockSdClk();
	GpioSdIoConfig(RESTORE_TO_GENERAL_IO);

	GpioClrRegBits(CARD_DETECT_PORT_PU, CARD_DETECT_BIT_MASK);
	GpioClrRegBits(CARD_DETECT_PORT_PD, CARD_DETECT_BIT_MASK);
	GpioClrRegBits(CARD_DETECT_PORT_OE, CARD_DETECT_BIT_MASK);

	GpioSetRegBits(CARD_DETECT_PORT_IE, CARD_DETECT_BIT_MASK);

	CardDetectDelay(); // ??? Disable????IE ?,??????

	if(GpioGetReg(CARD_DETECT_PORT_IN) & CARD_DETECT_BIT_MASK)
	{
		TempFlag = FALSE;
	}
	else
	{
		TempFlag = TRUE;
	}
	
	if(TempFlag)
	{
		GpioSdIoConfig(SD_PORT_NUM);
	}

	UnLockSdClk();

	return TempFlag;
}
bool IsUDiskLink(void)
{
	return UsbHost2IsLink();
}
static bool HardwareInit(DEV_ID DevId)
{
	switch(DevId)
	{
		case DEV_ID_SD:
			if(!IsCardLink())
			{
				return FALSE;
			}
			FSDeInit(DEV_ID_SD);
			if(SdCardInit())	
			{
				return FALSE;
			}
			if(!FSInit(DEV_ID_SD))
			{
				return FALSE;
			}
			return TRUE;
		case DEV_ID_USB:
			Usb2SetDetectMode(1, 0);
			UsbSetCurrentPort(UDISK_PORT_NUM);
			if(!IsUDiskLink())
			{
				return FALSE;
			}
			FSDeInit(DEV_ID_SD);
			FSDeInit(DEV_ID_USB);
			if(!HostStorInit())
			{
				return FALSE;
			}
			if(!FSInit(DEV_ID_USB))
			{
				return FALSE;
			}
			return TRUE;
		default:
			break;
	}
	return FALSE;
}
void FileBrowse(FS_CONTEXT* FsContext)
{	
	uint8_t	EntryType;
	DirSetStartEntry(FsContext, FsContext->gFsInfo.RootStart, 0, TRUE);
	FsContext->gFolderDirStart = FsContext->gFsInfo.RootStart;
	while(1)
	{
		EntryType = DirGetNextEntry(FsContext);
		switch(EntryType)
		{
			case ENTRY_FILE:
				DBG("%-.11s  %d年%d月%d日 %d:%d:%d  %d 字节\n",
					FsContext->gCurrentEntry->FileName,
					1980+(FsContext->gCurrentEntry->CreateDate >> 9),
					(FsContext->gCurrentEntry->CreateDate >> 5) & 0xF,
					(FsContext->gCurrentEntry->CreateDate) & 0x1F,
					FsContext->gCurrentEntry->CreateTime >> 11,
					(FsContext->gCurrentEntry->CreateTime >> 5) & 0x3F,
					((FsContext->gCurrentEntry->CreateTime << 1) & 0x3F) + (FsContext->gCurrentEntry->CrtTimeTenth / 100),
					FsContext->gCurrentEntry->Size);
				break;
			case ENTRY_FOLDER:
				FsContext->gFsInfo.FolderNumInFolder++;
				break;
			case ENTRY_END:
				RewindFolderStart(FsContext);
				if(!FindNextFolder(FsContext))
				{
					return;
				}
				FsContext->gFsInfo.FolderSum++;
				FsContext->gFsInfo.FolderNumInFolder = 0;
				FsContext->gFsInfo.FileSumInFolder = 0;
				if(!DirEnterSonFolder(FsContext))
				{
					FsContext->gFsInfo.FolderSum--;
				}
				break;
			default:
				break;
		}
	}
}
int32_t main(void)
{
//	uint8_t Name[128];
	ClkModuleEn(ALL_MODULE_CLK_SWITCH);	
	ClkModuleGateEn(ALL_MODULE_CLK_GATE_SWITCH);        //open all clk gating
	
	ClkPorRcToDpll(0); 		//clock src is 32768hz OSC
	CacheInit();	
	SysTickInit();
	
	WdgDis();
	GpioFuartRxIoConfig(1);
	GpioFuartTxIoConfig(1);
	FuartInit(115200, 8, 0, 1);

    DBG("/==========================================================================\\\n");
    DBG("|                           File System TESTBENCH                          |\n");
    DBG("| Shanghai Mountain View Silicon Technology Co.,Ltd. All Rights Reserved.  |\n");
    DBG("\\==========================================================================/\n");

	DBG("SdCard/UsbDevice detecting ...\n");
	while(1)
	{
		if(HardwareInit(DEV_ID_SD))//配置选择U盘或sd卡 默认为SD卡
		{
			DBG("Hardware initialize success.\n");
			break;
		}
		CardDetectDelay();
	}
	DBG("please input command as the following\n");
	DBG("Command   Discription\n");
	DBG("0   		format\n");
	DBG("1   		create one file named test1.txt\n");
	DBG("2   		create one folder named TEST\n");
	DBG("3   		create one long name folder named 12346789\n");
	DBG("4   		delete test1.txt\n");	
	DBG("5   		write 'test string' to test1.txt\n");
	DBG("6   		read 50 byre form test1.txt\n");
	DBG("7   		long file name test\n");
	DBG("s   		show file list of all disk\n");
	DBG("\n");
	while(1)
	{
		while(!GetKeyValue());
		switch(GetKeyValue())
		{
			case '0':// format
				DBG("0\n");
				DBG("Formating...\n");
				if(FSFormat())
				{
					DBG("Format success\n");
				}else
				{
					DBG("Format error\n");
				}
				break;
			case '1':// create one file named test1.txt
				DBG("1\n");
				FolderOpenByNum(&RootFolder, NULL, 1);
				if(FileOpen(&TestFile, "\\test1.txt", FA_CREATE_NEW))
				{
					FILE_TIME filetime;
					filetime.Year = 2014;
					filetime.Month = 8;
					filetime.Date = 21;
					filetime.Hour = 11;
					filetime.Min = 40;
					filetime.Sec = 21;
					FileSetTime(&TestFile, &filetime, NULL, NULL);
					DBG("FileCreate() success\n");
				}
				else
				{
					DBG("test1.txt already exist\n");
				}
				break;
			case '2':// create one folder named TEST
				DBG("2\n");
				FolderOpenByNum(&RootFolder, NULL, 1);
				if(!FolderOpenByName(&TestFolder, &RootFolder, "TEST"))
				{
					if(!FolderCreate(&TestFolder, &RootFolder, "TEST"))
					{
						DBG("FolderCreate()  Failed!\n");
					}else
					{
						DBG("FolderCreate() OK\n");
					}
				}
				else
				{
					DBG("Folder already exit!\n");
				}
				break;
			case '3':// create one long name folder named 12346789
				DBG("3\n");
				FolderOpenByNum(&RootFolder, NULL, 1);
			
				memset((void* )LongName, 0xFF, FAT_NAME_MAX);
				utf8_to_utf16(LongName, "123456789", FAT_NAME_MAX, 9);
				if(FolderCreateByLongName(&TestFolder, &RootFolder, LongName, FolderShortName, 9))
				{
					DBG("LongNameFolder Create Success!\n");
					if(FileOpen(&TestFile, "\\123456789\\test1.txt", FA_CREATE_NEW))
					{
						DBG("test.txt create success\n");
					}
					else
					{
						DBG("test.txt create failed\n");
					}
				}
				else
				{
					DBG("LongNameFolder Create Failed!\n");
				}
				break;
			case '4':// delete test1.txt
				DBG("4\n");
				if(FileOpen(&TestFile, "\\test1.txt",FA_READ))
				{
					if(!FileDelete(&TestFile))
					{
						DBG("FileDelete() Failed!\n");
					}else
					{
						DBG("FileDelete() Success\n");
					}
				}
				else
				{
					DBG("test1.txt not exist\n");
				}
				break;
			case '5':// write 'test string' to test1.txt
				DBG("5\n");
				if(FileOpen(&TestFile, "\\test1.txt", FA_WRITE))
				{
					WriteLen = FileWrite(Buf, 1, 50, &TestFile);
					DBG("%d Byte has been written\n", WriteLen);	
					FileClose(&TestFile);
				}
				else
				{
					DBG("test1.txt not exist\n");
				}
				break;
			case '6':// read 50 byre form test1.txt
				DBG("6\n");
				if(FileOpen(&TestFile, "\\test1.txt", FA_READ))
				{
					uint32_t i;
					ReadLen = FileRead(Buf2, 1, 50, &TestFile);
					DBG("%d Byte has been read\n", ReadLen);
					for(i = 0; i < 50; i++)
					{
						DBG("%c",Buf2[i]);
					}
					FileClose(&TestFile);
				}
				else
				{
					DBG("test1.txt not exist\n");
				}
				break;
			case '7':// long file name test
				DBG("7\n");
//				memset(Name, 0xFF, 128);
//				utf8_to_utf16((uint16_t *)Name, "test123456.txt", FAT_NAME_MAX, 14);
//				FolderOpenByNum(&RootFolder, NULL, 1);
//				if(FileCreateByLfName(&TestFile, &RootFolder, Name, 28))
//				{
//					uint8_t LongName[66];
//					DBG("FileCreateByLfName success!\n");
//					if(FileGetLongName(&TestFile, LongName, 66))
//					{
//						uint8_t i;
//						for(i = 0; i < 66; i++)
//						{
//							DBG("%02X ",LongName[i]);
//						}
//					}
//				}
//				else
//				{
//					DBG("FileCreateByLfName error!\n");
//				}
				
				if(FileOpen(&TestFile, "abcdefghijklllsadfssdfe.txt", FA_READ|FA_WRITE|FA_CREATE_ALWAYS))
				{			
					uint32_t i;
					DBG("FileOpen success!\n");
					WriteLen = FileWrite(Buf, 1, 50, &TestFile);
					DBG("%d Byte has been written\n", WriteLen);
					FileSave(&TestFile);
					FileSeek(&TestFile, 0, SEEK_FILE_SET);
					FileRead(Buf2, 1, 50, &TestFile);
					for(i = 0; i < 50; i++)
					{
						DBG("%c",Buf2[i]);
					}
					FileClose(&TestFile);
				}
				else
				{
					DBG("FileOpen Error!\n");
				}
				break;
			case 's':// show file list of all disk
				DBG("s\n");
				FolderOpenByNum(&RootFolder, NULL, 1);
				FileBrowse(RootFolder.FsContext);
				break;
			default:
				break;
		}
		ClrKeyValue();
		DBG("\n");
		DBG("please input command as the following\n");
		DBG("Command   Discription\n");
		DBG("0   		format\n");
		DBG("1   		create one file named test1.txt\n");
		DBG("2   		create one folder named TEST\n");
		DBG("3   		create one long name folder named 12346789\n");
		DBG("4   		delete test1.txt\n");	
		DBG("5   		write 'test string' to test1.txt\n");
		DBG("6   		read 50 byre form test1.txt\n");
		DBG("7   		long file name test\n");
		DBG("s   		show file list of all disk\n");
		DBG("\n");
	}
}
void FuartInterrupt(void)
{
    if(IsFuartRxIntSrc())
    {
        uint8_t c;        
        FuartRecvByte(&c);
        FuartClrRxInt();  
        SetKeyValue(c);
	}
	if(IsFuartTxIntSrc())
    {
        FuartClrTxInt();
	}
}

