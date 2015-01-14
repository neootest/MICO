/****
 * File: MICOAppEntrance.c
 * External Flash ,SPI flash Application with Mico os. Temporarily.
 * Jerry
 * Created @ Jan 6th 2015
 * */ 

#include "Mico.h"
#include "MicoPlatform.h"
#include "platform.h"
#include "platformInternal.h"
#include "platform_common_config.h"

//#define app_log(M, ...) custom_log("BT FL", M, ##__VA_ARGS__)
//#define app_log_trace() custom_log_trace("BOOT")

#define app_log     printf 


void Program_thread(){
  
  init_clocks();
  init_memory();
  init_architecture();
  init_platform_bootloader();

  app_log("wifi_image length");


    app_log("##DOWN FINISHED = \n");
 
}


int main(void){
    uint32_t wait = 0;
    Program_thread();
    
    while(1){
        if(wait == 1){
            app_log("device. ");
        }
        wait ++;
    }
    return 0;
}

