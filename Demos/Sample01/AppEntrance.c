/***
 * File: AppEntrace.c
 * a Sample Application.
 * 
 * */

#include "MicoPlatform.h"
#include "platform.h"
#include "platform_common_config.h"
#include "MICODefine.h"
#include "MICOAppDefine.h"


#define mico_log(M, ...) custom_log("MICO", M, ##__VA_ARGS__)



//int application_start(void){
OSStatus MICOStartApplication( mico_Context_t * const inContext ){
//    MICOStartApplication
  OSStatus err = kNoErr;
 
 // char wifi_ver[64];
 

//  MicoInit();
//  MicoSysLed(true);
  mico_log("Free memory %d bytes", MicoGetMemoryInfo()->free_memory) ; 
    
    return err; 
 }



