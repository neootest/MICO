#include "time.h"
#include "MicoPlatform.h"
#include "platform.h"
#include "platform_common_config.h"
#include "MICODefine.h"
#include "MICOAppDefine.h"
#include "MICONotificationCenter.h"



#define MFG_FUNCTION             2



/* MFG test demo BEGIN */
extern int mfg_connect(char *ssid);
extern int mfg_scan(void);
extern void mfg_option(int use_udp, uint32_t remoteaddr);
extern char* system_lib_version(void);
extern void wlan_get_mac_address(char *mac);

static char cmd_str[64];

void mf_printf(char *str)
{
    MicoUartSend( MFG_TEST, str, strlen(str));
}

static void mf_putc(char ch)
{
    MicoUartSend( MFG_TEST, &ch, 1);
}

static int get_line()
{
#define CNTLQ      0x11
#define CNTLS      0x13
#define DEL        0x7F
#define BACKSPACE  0x08
#define CR         0x0D
#define LF         0x0A

	char *p = cmd_str;
	int i = 0;
	char c;

	memset(cmd_str, 0, sizeof(cmd_str));
	while(1) {
		if( MicoUartRecv( MFG_TEST, p, 1, 100) != kNoErr)
			continue;
		
		mf_putc(*p);
		if (*p == BACKSPACE  ||  *p == DEL)  {
			if(i>0) {
				c = 0x20;
				mf_putc(c); 
				mf_putc(*p); 
				p--;
				i--; 
			}
			continue;
		}
		if(*p == CR || *p == LF) {
			*p = 0;
			return i;
		}

		p++;
		i++;
		if (i>sizeof(cmd_str))
			break;
	}
	
	return 0;
}


/**
  * @brief  Display the Main Menu on HyperTerminal
  * @param  None
  * @retval None
  */
static char * ssid_get(void)
{
	char *cmd;
    int is_use_udp = 1;
    uint32_t remote_addr = 0xFFFFFFFF;
    
	while (1)  {                                 /* loop forever                */
		mf_printf ("\r\nMXCHIP_MFMODE> ");
		get_line();
        cmd = cmd_str;
		if (strncmp(cmd, "tcp ", 4) == 0) {
			mf_printf ("\r\n");
			remote_addr = inet_addr(cmd+4);
			if (remote_addr == 0)
				remote_addr = 0xffffffff;
			sprintf(cmd, "Use TCP send packet to 0x%X\r\n", (unsigned int)remote_addr);
			mf_printf (cmd);
			is_use_udp = 0;
		} else if (strncmp(cmd, "udp ", 4) == 0) {
			mf_printf ("\r\n");
			remote_addr = inet_addr(cmd+4);
			if (remote_addr == 0)
				remote_addr = 0xffffffff;
			sprintf(cmd, "Use UDP send packet to 0x%X\r\n", (unsigned int)remote_addr);
			mf_printf (cmd);
		}  else if (strncmp(cmd, "ssid ", 5) == 0) {
			mf_printf ("\r\n");
			return cmd+5;
		} else {
			mf_printf ("Please input as \"ssid <ssid_string>\"");
			continue;
		}
	}
    
    mfg_option(is_use_udp, remote_addr);
}

#if (MFG_FUNCTION == 1) 
void mico_mfg_test(void)
{
    char str[64];
    char mac[6];
    char *ssid;
  
    sprintf(str, "Library Version: %s\r\n", system_lib_version());
	mf_printf(str);
    mf_printf("APP Version: ");
    memset(str, 0, sizeof(str));
    system_version(str, sizeof(str));
    mf_printf(str);
    mf_printf("\r\n");
    memset(str, 0, sizeof(str));
    wlan_driver_version(str, sizeof(str));
    mf_printf("Driver: ");
    mf_printf(str);
    mf_printf("\r\n");
	wlan_get_mac_address(mac);
	sprintf(str, "MAC: %02X-%02X-%02X-%02X-%02X-%02X\r\n",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	mf_printf(str);

    mfg_scan();

    ssid = ssid_get();
    mfg_connect(ssid);
    
    mico_thread_sleep(MICO_NEVER_TIMEOUT);
}
#elif (MFG_FUNCTION == 2)
void mico_mfg_test(void)
{
    char str[64];
    char mac[6];
    char *ssid;
  
    sprintf(str, "Library Version  222: %s\r\n", system_lib_version());
	mf_printf(str);
    mf_printf("APP Version: ");
    memset(str, 0, sizeof(str));
    system_version(str, sizeof(str));
    mf_printf(str);
    mf_printf("\r\n");
    memset(str, 0, sizeof(str));
    wlan_driver_version(str, sizeof(str));
    mf_printf("Driver: ");
    mf_printf(str);
    mf_printf("\r\n");
	wlan_get_mac_address(mac);
	sprintf(str, "MAC: %02X-%02X-%02X-%02X-%02X-%02X\r\n",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	mf_printf(str);

    mfg_scan();

    ssid = ssid_get();
    mfg_connect(ssid);
    
    mico_thread_sleep(MICO_NEVER_TIMEOUT);
}
#endif
/* MFG test demo END */

