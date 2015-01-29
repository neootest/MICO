;/**************************************************************************;**
; * @file     startup_ARMCM3.s
; * @brief    CMSIS Core Device Startup File for
; *           ARMCM3 Device Series
; * @version  V1.08
; * @date     23. November 2012
; *
; * @note
; *
; ******************************************************************************/
;/* Copyright (c) 2011 - 2012 ARM LIMITED
;
;   All rights reserved.
;   Redistribution and use in source and binary forms, with or without
;   modification, are permitted provided that the following conditions are met:
;   - Redistributions of source code must retain the above copyright
;     notice, this list of conditions and the following disclaimer.
;   - Redistributions in binary form must reproduce the above copyright
;     notice, this list of conditions and the following disclaimer in the
;     documentation and/or other materials provided with the distribution.
;   - Neither the name of ARM nor the names of its contributors may be used
;     to endorse or promote products derived from this software without
;     specific prior written permission.
;   *
;   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
;   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
;   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
;   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
;   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
;   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
;   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
;   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
;   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
;   POSSIBILITY OF SUCH DAMAGE.
;   ---------------------------------------------------------------------------*/
;/*
;;-------- <<< Use Configuration Wizard in Context Menu >>> ------------------
;*/


; <h> Stack Configuration
;   <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

Stack_Size      EQU     0x00000200

                AREA    MSP, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp


; <h> Heap Configuration
;   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

;Heap_Size       EQU     0x00012000

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
;__heap_base
;Heap_Mem        SPACE   Heap_Size
;__heap_limit
__heap_base     EQU     (0x20000000 + Stack_Size)
__heap_limit    EQU     (0x20012000 + Stack_Size)


                PRESERVE8
                THUMB


; Vector Table Mapped to Address 0 at Reset

                AREA    EXCEPT_VECTS, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_sp              ; Top of Stack
                DCD     Reset_Handler             ; Reset Handler
                DCD     NMI_Handler               ; NMI Handler
                DCD     HardFault_Handler         ; Hard Fault Handler
                DCD     MemManage_Handler         ; MPU Fault Handler
                DCD     BusFault_Handler          ; Bus Fault Handler
                DCD     UsageFault_Handler        ; Usage Fault Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     SVC_Handler               ; SVCall Handler
                DCD     0                         ; Debug Monitor Handler
                DCD     0                         ; Reserved
                DCD     PendSV_Handler            ; PendSV Handler
                DCD     SysTick_Handler           ; SysTick Handler

                ; External Interrupts
                DCD     Gpio_IRQHandler           ;  
                DCD     RTC_IRQHandler            ; 
                DCD     Ir_IRQHandler             ;
                DCD     Fuart_IRQHandler          ; 
                DCD     Buart_IRQHandler          ; 
                DCD     Pwc_IRQHandler            ; 
                DCD     TIM0_IRQHandler           ; 
                DCD     Usb_IRQHandler            ; 
                DCD     DmaCh0_IRQHandler         ; 
                DCD     DmaCh1_IRQHandler         ; 
                DCD     audio_decoder_IRQHandler  ; 
                DCD     Spis_IRQHandler           ; 
                DCD     Sd_IRQHandler             ; 
                DCD     Spim_IRQHandler           ; 
                DCD     Timer1_IRQHandler         ; 
                DCD     WatchDog_IRQHandler       ; 
__Vectors_End

__Vectors_Size  EQU     __Vectors_End - __Vectors

	;image total size in bytes
	AREA    |.ARM.__at_0x00000080|, CODE, READONLY
    IMAGE_SIZE	DCD	0xFFFFFFFF

	;product ID & Check sum
	AREA    |.ARM.__at_0x00000084|, CODE, READONLY
    PIDCHKSUM	FILL 8, 0xFF


	;constant data offset
	AREA    |.ARM.__at_0x0000008C|, CODE, READONLY
    CONSTDATA	DCD	0x100000

	;user data offset
	AREA    |.ARM.__at_0x00000090|, CODE, READONLY
    USERDATA	DCD	0x1A0000

	;sdk version
	AREA    |.ARM.__at_0x00000094|, CODE, READONLY
    SDK_VER_CHIPID	DCB		0x12    ;MV chip ID
    SDK_VER_MAJOR	DCB		2       ;MV SDK major version
    SDK_VER_MINOR	DCB		0      ;MV SDK minor version
    SDK_VER_USER	DCB		1       ;user SDK version

	;code memory(exclude of this 4 bytes) crc
	AREA    |.ARM.__at_0x00000098|, CODE, READONLY
    SDK_CODECRC	DCD	0xFFFFFFFF

    ;code magic number
	AREA    |.ARM.__at_0x0000009C|, CODE, READONLY
    SDK_CODEMGC	DCD	0xB0BEBDC9

    ;32KHz external oscillator input/output capacitance calibration value
	AREA    |.ARM.__at_0x000000A0|, CODE, READONLY
    OSC32K_CAP	DCD	0x00000706

    AREA    |.ARM.__at_0x000000A4|, CODE, READONLY    
    USER_RESV_ZOOM    FILL  (0xFC - 0xA4),    0xFF

    AREA    |.ARM.__at_0x000000FC|, CODE, READONLY    
    CODE_ENCRYP_FLAG	DCD  0xFFFFFFFF

                AREA    |.text|, CODE, READONLY

	IMPORT	|Region$$Table$$Base|
	IMPORT	|Region$$Table$$Limit|

	IMPORT  main
	IF :DEF:OS_VERSION
	IMPORT	mmm_pool_top 
	ENDIF
		;the follow code crack the uvision startup code -huangyucai20111018
__mv_main
		;set up the system stack
	IF :DEF:OS_VERSION
		LDR		SP,=mmm_pool_top 
	ELSE
;		LDR		SP,=0x20008000
	ENDIF 
		SUB		SP,SP,#0x1000
		LDR		R0,=__initial_sp
		SUB		R0,R0,#Stack_Size
		LDR		R0,[R0]
		PUSH	{R0}

mv_main	PROC
		EXPORT	mv_main	
			
		;get the load region layout table
		LDR		R4,=|Region$$Table$$Base|
		LDR		R5,=|Region$$Table$$Limit|
__NEXT_REGION
		CMP		R4,R5
		;everything is ok
		BCS		__REGION_DECOMP_OK
		LDM		R4,{R0-R3}
		;decompress the data following the compress algorithm as compiling method
		STMDB	R13!,{R4,R5}
		ORR		R3,R3,#0x01
		BLX 	R3
		LDMIA	R13!,{R4,R5}
		ADD		R4,R4,#0x10
		B		__NEXT_REGION

__REGION_DECOMP_OK
	IF :DEF:__MICROLIB

	ELSE
		IMPORT	__rt_lib_init 
		BL		__rt_lib_init
	ENDIF
;__MICROLIB

		;fill the system stack space with debug symbol for debug only -huangyucai20111121
	IF :DEF:CFG_SHELL_DEBUG
;		LDR		R2,=Stack_Size
;		LDR		R3,=__initial_sp
;		SUB		R3,R3,#1
;		MOV		R4,#0xA5
;AGAIN
;		STRB	R4,[R3],#-0x01
;		SUBS	R2,R2,#0x01
;		BHI		AGAIN
	ENDIF 
;CFG_SHELL_DEBUG
        
        CPSIE   I
        CPSIE   F
		LDR		SP,=__initial_sp
		LDR		R0,=main
		BX		R0

		ENDP
        ALIGN

; Reset Handler

Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
;                IMPORT  SystemInit
;                IMPORT  __main
;                LDR     R0, =SystemInit
;                BLX     R0
		        LDR		R0,=__mv_main
 ;               LDR     R0, =__main
                BX      R0
                ENDP


; Dummy Exception Handlers (infinite loops which can be modified)

NMI_Handler     PROC
                EXPORT  NMI_Handler               [WEAK]
                B       .
                ENDP
HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler         [WEAK]
                IMPORT  hard_fault_handler_c
                TST LR, #4
             ;   ITE EQ
                MRSEQ R0, MSP
                MRSNE R0, PSP
		        PUSH	{R4-R11}
		        MRS		R1,	MSP
                B       hard_fault_handler_c
                ENDP
MemManage_Handler\
                PROC
                EXPORT  MemManage_Handler         [WEAK]
		        IMPORT hard_fault_handler_c
		        TST		LR,	#4
                ITE EQ
		        MRSEQ	R0,	MSP
		        MRSNE	R0,	PSP
		      ;  PUSH	{R4-R11}
		      ;  MRS		R1,	MSP
		        B		hard_fault_handler_c
                ENDP
BusFault_Handler\
                PROC
                EXPORT  BusFault_Handler          [WEAK]
		        IMPORT hard_fault_handler_c
		        TST		LR,	#4
                ITE EQ
		        MRSEQ	R0,	MSP
		        MRSNE	R0,	PSP
		        ;PUSH	{R4-R11}
		        ;MRS		R1,	MSP
		        B		hard_fault_handler_c
                ENDP
UsageFault_Handler\
                PROC
                EXPORT  UsageFault_Handler        [WEAK]
		        IMPORT hard_fault_handler_c
		        TST		LR,	#4
		        MRSEQ	R0,	MSP
		        MRSNE	R0,	PSP
		        PUSH	{R4-R11}
		        MRS		R1,	MSP
		        B		hard_fault_handler_c
                ENDP
SVC_Handler     PROC
                EXPORT  SVC_Handler               [WEAK]
				IMPORT vPortSVCHandler
                B vPortSVCHandler
                ENDP
PendSV_Handler  PROC
                EXPORT  PendSV_Handler            [WEAK]
				IMPORT xPortPendSVHandler
                B xPortPendSVHandler
                ENDP
SysTick_Handler PROC
                EXPORT  SysTick_Handler           [WEAK]
                IMPORT xPortSysTickHandler
                B xPortSysTickHandler
                ENDP

Default_Handler PROC

                EXPORT     Gpio_IRQHandler           [WEAK]  
                EXPORT     RTC_IRQHandler            [WEAK] 
                EXPORT     Ir_IRQHandler             [WEAK]
                EXPORT     Fuart_IRQHandler          [WEAK] 
                EXPORT     Buart_IRQHandler          [WEAK] 
                EXPORT     Pwc_IRQHandler            [WEAK] 
                EXPORT     TIM0_IRQHandler           [WEAK] 
                EXPORT     Usb_IRQHandler            [WEAK] 
                EXPORT     DmaCh0_IRQHandler         [WEAK] 
                EXPORT     DmaCh1_IRQHandler         [WEAK] 
                EXPORT     audio_decoder_IRQHandler  [WEAK] 
                EXPORT     Spis_IRQHandler           [WEAK] 
                EXPORT     Sd_IRQHandler             [WEAK] 
                EXPORT     Spim_IRQHandler           [WEAK] 
                EXPORT     Timer1_IRQHandler         [WEAK] 
                EXPORT     WatchDog_IRQHandler       [WEAK] 

Gpio_IRQHandler
RTC_IRQHandler
Ir_IRQHandler
Fuart_IRQHandler
Buart_IRQHandler
Pwc_IRQHandler
TIM0_IRQHandler
Usb_IRQHandler
DmaCh0_IRQHandler
DmaCh1_IRQHandler
audio_decoder_IRQHandler
Spis_IRQHandler
Sd_IRQHandler
Spim_IRQHandler
Timer1_IRQHandler
WatchDog_IRQHandler
                
                B       .

                ENDP


                ALIGN


; User Initial Stack & Heap

                IF      :DEF:__MICROLIB

                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit

                ELSE

                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit
;                IMPORT  __use_two_region_memory
;                EXPORT  __user_initial_stackheap

;__user_initial_stackheap PROC
;                LDR     R0, =  Heap_Mem
;                LDR     R1, =(Stack_Mem + Stack_Size)
;                LDR     R2, = (Heap_Mem +  Heap_Size)
;                LDR     R3, = Stack_Mem
;                BX      LR
;                ENDP

                ALIGN

                ENDIF


                END
