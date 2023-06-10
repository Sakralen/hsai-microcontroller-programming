#define KB_FLAGS_ONLY_CFG

//include directives:
#include "includes.h"
#include <stm32f10x.h>
//!include directives

//define directives:
#define KEY_FLAG_1					(u16)(1<<0)
#define KEY_FLAG_2					(u16)(1<<1)
#define KEY_FLAG_3					(u16)(1<<2)
#define KEY_FLAG_4					(u16)(1<<3)
#define KEY_FLAG_5					(u16)(1<<4)
#define KEY_FLAG_6					(u16)(1<<5)
#define KEY_FLAG_7					(u16)(1<<6)
#define KEY_FLAG_8					(u16)(1<<7)
#define KEY_FLAG_9					(u16)(1<<8)

#define SND_MIN_DLY					3
#define SND_DELTA_DLY 			2

#define SND_DURATION				100 
//!define directives

//function declarations:
static void App_TaskStart (void *p_arg);
static void App_TaskSound (void *p_arg);
static void App_TaskKeyPress (void *p_arg);
static void App_TaskKeyPoll (void *p_arg);
//!function declarations

//global variables:
//task stacks:
OS_STK App_TaskStartStk[APP_TASK_STK_SIZE];
OS_STK App_TaskSoundStk[APP_TASK_STK_SIZE];
OS_STK App_TaskKeyPressStk[APP_TASK_STK_SIZE];
OS_STK App_TaskKeyPollStk[APP_TASK_STK_SIZE];
//!task stacks

OS_EVENT*     pMbox;
OS_FLAG_GRP*  pFlagGroup;

u8 cTicks;
u8 err;
//!global variables

int  main (void) {
	BSP_IntDisAll();
	OSInit();
  OSTaskCreate((void (*)(void *)) App_TaskStart,
               (void          * ) 0,
               (OS_STK        * )&App_TaskStartStk[APP_TASK_STK_SIZE - 1],
               (INT8U           ) APP_TASK_START_PRIO
              );
							 
  OSStart();              
  return 0;
}

static void App_TaskStart (void *p_arg) {
	BSP_Init();

	OS_CPU_SysTickInit();
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_InitTypeDef port; 
	
	GPIO_StructInit(&port); //Выходы для клавиатуры
	port.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
	port.GPIO_Mode = GPIO_Mode_Out_OD;
	port.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &port);
	
	GPIO_StructInit(&port);	//Входы для клавиатуры
	port.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
	port.GPIO_Mode = GPIO_Mode_IPU;
	port.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &port);
	
	GPIO_StructInit(&port);	//Выход для пищалки
	port.GPIO_Pin = GPIO_Pin_6;
	port.GPIO_Mode = GPIO_Mode_Out_PP;
	port.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &port);
	
	pMbox = OSMboxCreate(0);
  pFlagGroup = OSFlagCreate(0, &err);
	
	err = OSTaskCreate((void (*)(void *)) App_TaskSound,
											 (void          * ) 0,
											 (OS_STK        * ) &App_TaskSoundStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_SOUND_PRIO
											);
	if (err) {
		__NOP();
	}
	
	err = OSTaskCreate((void (*)(void *)) App_TaskKeyPress,
											 (void          * ) 0,
											 (OS_STK        * ) &App_TaskKeyPressStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_KEY_PRESS_PRIO	
											);
	if (err) {
		__NOP();
	}
	
	err = OSTaskCreate((void (*)(void *)) App_TaskKeyPoll,
											 (void          * ) 0,
											 (OS_STK        * ) &App_TaskKeyPollStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_KEY_POLL_PRIO
											);
	if (err) {
		__NOP();
	}
	
	OSTaskDel(APP_TASK_START_PRIO);
	
	while(DEF_TRUE) {
	
	}
}

static void App_TaskSound (void *p_arg) {
	u32 sndDly;
	u32 sndDuration;
	while(DEF_TRUE) {
		sndDly = *((u32*)OSMboxPend(pMbox, 0, &err));
		sndDuration = SND_DURATION;
		do {
			GPIO_SetBits(GPIOA, GPIO_Pin_6);
			OSTimeDly(sndDly);
			GPIO_ResetBits(GPIOA, GPIO_Pin_6);
			OSTimeDly(sndDly);
		} while(sndDuration--);
	}
}

static void App_TaskKeyPress (void *p_arg) {
	u32 sndDly;
	OS_FLAGS flagVal;
	while(DEF_TRUE) {
		OSFlagPend(pFlagGroup, 	KEY_FLAG_1 | KEY_FLAG_2 | KEY_FLAG_3 | 
														KEY_FLAG_4 | KEY_FLAG_5 | KEY_FLAG_6 |
														KEY_FLAG_7 | KEY_FLAG_8 | KEY_FLAG_9, OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 0, &err);
		flagVal = OSFlagPendGetFlagsRdy();
		
		switch(flagVal) {
			case KEY_FLAG_1:
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 0;
				break;
			case KEY_FLAG_2:
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 1;
				break;
			case KEY_FLAG_3:
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 2;
				break;
			case KEY_FLAG_4:
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 3;
				break;
			case KEY_FLAG_5:
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 4;
				break;
			case KEY_FLAG_6:
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 5;
				break;
			case KEY_FLAG_7:
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 6;
				break;
			case KEY_FLAG_8:
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 7;
				break;
			case KEY_FLAG_9:
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 8;
				break;
			default:
				break;
		}		
		OSMboxPost(pMbox, &sndDly);
	}
}

static void App_TaskKeyPoll (void *p_arg) {
	GPIO_SetBits(GPIOC, GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12);
	while(DEF_TRUE) {
		GPIO_SetBits(GPIOC, GPIO_Pin_12);
		GPIO_ResetBits(GPIOC, GPIO_Pin_10);
		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2) == RESET) {
			OSFlagPost(pFlagGroup, KEY_FLAG_1, OS_FLAG_SET, &err);
		}
		else if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1) == RESET) {
			OSFlagPost(pFlagGroup, KEY_FLAG_2, OS_FLAG_SET, &err);
		}
		else if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0) == RESET) {
			OSFlagPost(pFlagGroup, KEY_FLAG_3, OS_FLAG_SET, &err);
		}
		
		GPIO_SetBits(GPIOC, GPIO_Pin_10);
		GPIO_ResetBits(GPIOC, GPIO_Pin_11);
		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2) == RESET) {
			OSFlagPost(pFlagGroup, KEY_FLAG_4, OS_FLAG_SET, &err);
		}
		else if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1) == RESET) {
			OSFlagPost(pFlagGroup, KEY_FLAG_5, OS_FLAG_SET, &err);
		}
		else if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0) == RESET) {
			OSFlagPost(pFlagGroup, KEY_FLAG_6, OS_FLAG_SET, &err);
		}
			
		GPIO_SetBits(GPIOC, GPIO_Pin_11);
		GPIO_ResetBits(GPIOC, GPIO_Pin_12);
		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2) == RESET) {
			OSFlagPost(pFlagGroup, KEY_FLAG_7, OS_FLAG_SET, &err);
		}
		else if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1) == RESET) {
			OSFlagPost(pFlagGroup, KEY_FLAG_8, OS_FLAG_SET, &err);	
		}
		else if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0) == RESET) {
			OSFlagPost(pFlagGroup, KEY_FLAG_9, OS_FLAG_SET, &err);
		}

		OSTimeDly(2);
	}
}

