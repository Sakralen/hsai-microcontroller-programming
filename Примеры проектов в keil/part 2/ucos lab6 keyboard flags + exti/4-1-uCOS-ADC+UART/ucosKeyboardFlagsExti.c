#define KB_FLAGS_EXTI_CFG

//include directives:
#include "includes.h"
#include <stm32f10x.h>
//!include directives

//define directives:
#define BUZZER_PORT 			GPIOA
#define KB_PORT						GPIOC

#define BUZZER_PIN				GPIO_Pin_6
#define KB_COL_1_PIN			GPIO_Pin_10
#define KB_COL_2_PIN			GPIO_Pin_11
#define KB_COL_3_PIN			GPIO_Pin_12
#define KB_ROW_1_PIN			GPIO_Pin_0
#define KB_ROW_2_PIN			GPIO_Pin_1
#define KB_ROW_3_PIN			GPIO_Pin_2
#define KB_ROW_4_PIN			GPIO_Pin_3

#define KB_PORT_SOURCE  	GPIO_PortSourceGPIOC 
#define KB_PIN_SOURCE_1		GPIO_PinSource0
#define KB_PIN_SOURCE_2		GPIO_PinSource1
#define KB_PIN_SOURCE_3		GPIO_PinSource2
#define KB_PIN_SOURCE_4		GPIO_PinSource3

#define EXTI0_IRQ_PRIO			4
#define EXTI1_IRQ_PRIO			5
#define EXTI2_IRQ_PRIO			6
#define EXTI3_IRQ_PRIO			7

//#define KB_COL_1_FLAG_MASK	1<<0
//#define KB_COL_2_FLAG_MASK	1<<1
//#define KB_COL_3_FLAG_MASK	1<<2	
#define KB_ROW_1_FLAG_MASK	1<<3
#define KB_ROW_2_FLAG_MASK	1<<4
#define KB_ROW_3_FLAG_MASK	1<<5
#define KB_ROW_4_FLAG_MASK	1<<6

#define SND_MIN_DLY					3
#define SND_DELTA_DLY 			2

#define SND_DURATION				100 
//!define directives

//function declarations:
static void App_TaskStart (void *p_arg);
static void App_TaskSound (void *p_arg);
static void App_TaskKeyPress (void *p_arg);
static void App_TaskKeyPoll (void *p_arg);

void EXTI0_IrqHandler(void);
void EXTI1_IrqHandler(void);
void EXTI2_IrqHandler(void);
void EXTI3_IrqHandler(void);
//function declarations

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

u8 i;

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

	BSP_IntVectSet(BSP_INT_ID_EXTI0, EXTI0_IrqHandler);
	BSP_IntVectSet(BSP_INT_ID_EXTI1, EXTI1_IrqHandler);
	BSP_IntVectSet(BSP_INT_ID_EXTI2, EXTI2_IrqHandler);
	BSP_IntVectSet(BSP_INT_ID_EXTI3, EXTI3_IrqHandler);
	
	NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = EXTI0_IRQ_PRIO;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
  NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn ;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = EXTI1_IRQ_PRIO;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
  NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn ;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = EXTI2_IRQ_PRIO;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
  NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn ;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = EXTI3_IRQ_PRIO;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	OS_CPU_SysTickInit();
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_InitTypeDef port; 
	
	GPIO_StructInit(&port); //Выходы для клавиатуры
	port.GPIO_Pin = KB_COL_1_PIN | KB_COL_2_PIN | KB_COL_3_PIN;
	port.GPIO_Mode = GPIO_Mode_Out_OD;
	port.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(KB_PORT, &port);
	
	GPIO_StructInit(&port);	//Входы для клавиатуры
	port.GPIO_Pin = KB_ROW_1_PIN | KB_ROW_2_PIN | KB_ROW_3_PIN | KB_ROW_4_PIN;
	port.GPIO_Mode = GPIO_Mode_IPU;
	port.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(KB_PORT, &port);
	
	GPIO_StructInit(&port);	//Выход для пищалки
	port.GPIO_Pin = BUZZER_PIN;
	port.GPIO_Mode = GPIO_Mode_Out_PP;
	port.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(BUZZER_PORT, &port);
	
	GPIO_EXTILineConfig(KB_PORT_SOURCE, KB_PIN_SOURCE_1);
	GPIO_EXTILineConfig(KB_PORT_SOURCE, KB_PIN_SOURCE_2);
	GPIO_EXTILineConfig(KB_PORT_SOURCE, KB_PIN_SOURCE_3);
	GPIO_EXTILineConfig(KB_PORT_SOURCE, KB_PIN_SOURCE_4);
	
	EXTI_InitTypeDef   EXTI_InitStructure;
	EXTI_InitStructure.EXTI_Line = EXTI_Line0 | EXTI_Line1 | EXTI_Line2 | EXTI_Line3;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
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
			GPIO_SetBits(BUZZER_PORT, BUZZER_PIN);
			OSTimeDly(sndDly);
			GPIO_ResetBits(BUZZER_PORT, BUZZER_PIN);
			OSTimeDly(sndDly);
		} while(sndDuration--);
	}
}

static void App_TaskKeyPress (void *p_arg) {
	u32 sndDly;
	OS_FLAGS flagVal;
	while(DEF_TRUE) {
		OSFlagPend(pFlagGroup, KB_ROW_1_FLAG_MASK | KB_ROW_2_FLAG_MASK | KB_ROW_3_FLAG_MASK | KB_ROW_4_FLAG_MASK, OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 0, &err);
		flagVal = OSFlagPendGetFlagsRdy();
//		flagVal = OSFlagQuery(pFlagGroup, &err);
		
		if (KB_ROW_1_FLAG_MASK == flagVal) {
			if (RESET == GPIO_ReadOutputDataBit(KB_PORT, KB_COL_1_PIN)) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 0;
			}
			else if (RESET == GPIO_ReadOutputDataBit(KB_PORT, KB_COL_2_PIN)) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 1;
			}
			else if (RESET == GPIO_ReadOutputDataBit(KB_PORT, KB_COL_3_PIN)) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 2;
			}	
		}
		
		else if (KB_ROW_2_FLAG_MASK == flagVal) {
			if (RESET == GPIO_ReadOutputDataBit(KB_PORT, KB_COL_1_PIN)) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 3;
			}
			else if (RESET == GPIO_ReadOutputDataBit(KB_PORT, KB_COL_2_PIN)) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 4;
			}
			else if (RESET == GPIO_ReadOutputDataBit(KB_PORT, KB_COL_3_PIN)) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 5;
			}	
		}
		
		else if (KB_ROW_3_FLAG_MASK == flagVal) {
			if (RESET == GPIO_ReadOutputDataBit(KB_PORT, KB_COL_1_PIN)) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 6;
			}
			else if (RESET == GPIO_ReadOutputDataBit(KB_PORT, KB_COL_2_PIN)) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 7;
			}
			else if (RESET == GPIO_ReadOutputDataBit(KB_PORT, KB_COL_3_PIN)) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 8;
			}	
		}
		
		else if (KB_ROW_4_FLAG_MASK == flagVal) {
			if (RESET == GPIO_ReadOutputDataBit(KB_PORT, KB_COL_1_PIN)) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 9;
			}
			else if (RESET == GPIO_ReadOutputDataBit(KB_PORT, KB_COL_2_PIN)) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 10;
			}
			else if (RESET == GPIO_ReadOutputDataBit(KB_PORT, KB_COL_3_PIN)) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 11;
			}	
		}
		
		OSMboxPost(pMbox, &sndDly);
	}
}

static void App_TaskKeyPoll (void *p_arg) {
	OS_CPU_SR  cpu_sr = 0;
	GPIO_SetBits(KB_PORT, KB_COL_1_PIN | KB_COL_2_PIN | KB_COL_3_PIN);
	while(DEF_TRUE) {
		
		//OS_ENTER_CRITICAL();
		GPIO_ResetBits(KB_PORT, KB_COL_1_PIN);
		OSTimeDly(2);
		GPIO_SetBits(KB_PORT, KB_COL_1_PIN);
		//OS_EXIT_CRITICAL();
		OSTimeDly(5);
		
		//OS_ENTER_CRITICAL();
		GPIO_ResetBits(KB_PORT, KB_COL_2_PIN);
		OSTimeDly(2);
		GPIO_SetBits(KB_PORT, KB_COL_2_PIN);
		//OS_EXIT_CRITICAL();
		OSTimeDly(5);
		
		//OS_ENTER_CRITICAL();
		GPIO_ResetBits(KB_PORT, KB_COL_3_PIN);
		OSTimeDly(2);
		GPIO_SetBits(KB_PORT, KB_COL_3_PIN);
		//OS_EXIT_CRITICAL();
		OSTimeDly(5);
		
		OSTimeDly(OS_TICKS_PER_SEC / 4);
	}
}
	
void EXTI0_IrqHandler(void) {
	OSIntEnter();
	EXTI_ClearITPendingBit(EXTI_Line0);
	OSFlagPost(pFlagGroup, KB_ROW_1_FLAG_MASK, OS_FLAG_SET, &err);
	OSIntExit();
}
void EXTI1_IrqHandler(void) {
	OSIntEnter();
	EXTI_ClearITPendingBit(EXTI_Line1);
	OSFlagPost(pFlagGroup, KB_ROW_2_FLAG_MASK, OS_FLAG_SET, &err);
	OSIntExit();
}
void EXTI2_IrqHandler(void) {
	OSIntEnter();
	EXTI_ClearITPendingBit(EXTI_Line2);
	OSFlagPost(pFlagGroup, KB_ROW_3_FLAG_MASK, OS_FLAG_SET, &err);
	OSIntExit();
}
void EXTI3_IrqHandler(void) {
	OSIntEnter();
	EXTI_ClearITPendingBit(EXTI_Line3);
	OSFlagPost(pFlagGroup, KB_ROW_4_FLAG_MASK, OS_FLAG_SET, &err);
	OSIntExit();
}	
