/***
 * File: stm32_platform.c
 * test code
 * JerryYu
 * Create @ Jan 12nd 2015
 * Version: 0.1
 * */

#include "stm32_platform.h"


int main(void){
  init_clocks();
  init_memory();
  init_architecture();
  init_platform_bootloader();
	
	while(1){
	
	}
}

