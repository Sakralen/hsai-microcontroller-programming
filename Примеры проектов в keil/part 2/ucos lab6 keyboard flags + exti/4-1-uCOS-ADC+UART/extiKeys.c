#define EXTI_ONLY_CFG

#include "includes.h"
#include <stm32f10x.h>

#define SND_MIN_DLY					3
#define SND_DELTA_DLY 			2
#define SND_DURATION				100 

#define EXTI0_IRQ_PRIO			4
#define EXTI1_IRQ_PRIO			5

static void App_TaskStart (void *p_arg);
static void App_TaskSound (void *p_arg);

void EXTI0_IrqHandler(void);
void EXTI1_IrqHandler(void);

OS_STK App_TaskStartStk[APP_TASK_STK_SIZE];
OS_STK App_TaskSoundStk[APP_TASK_STK_SIZE];

OS_EVENT* pMbox;

u8 cTicks;
u8 err;

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
	
	NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = EXTI0_IRQ_PRIO;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
  NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn ;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = EXTI1_IRQ_PRIO;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	OS_CPU_SysTickInit();
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_InitTypeDef port; 
	
	GPIO_StructInit(&port);
	port.GPIO_Pin = GPIO_Pin_10;
	port.GPIO_Mode = GPIO_Mode_Out_OD;
	port.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &port);
	
	GPIO_StructInit(&port);
	port.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	port.GPIO_Mode = GPIO_Mode_IPU;
	port.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &port);
	
		
	GPIO_StructInit(&port);	//Выход для пищалки
	port.GPIO_Pin = GPIO_Pin_6;
	port.GPIO_Mode = GPIO_Mode_Out_PP;
	port.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &port);
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource0);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource1);
	
	EXTI_InitTypeDef   EXTI_InitStructure;
	EXTI_InitStructure.EXTI_Line = EXTI_Line0 | EXTI_Line1;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
	
	pMbox = OSMboxCreate(0);
	
	err = OSTaskCreate((void (*)(void *)) App_TaskSound,
											 (void          * ) 0,
											 (OS_STK        * ) &App_TaskSoundStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_SOUND_PRIO
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
		sndDly = (u32)OSMboxPend(pMbox, 0, &err);
		sndDuration = SND_DURATION;
		do {
			GPIO_SetBits(GPIOA, GPIO_Pin_6);
			OSTimeDly(sndDly);
			GPIO_ResetBits(GPIOA, GPIO_Pin_6);
			OSTimeDly(sndDly);
		} while(sndDuration--);
	}
}

void EXTI0_IrqHandler(void) {
	OSIntEnter();
	EXTI_ClearITPendingBit(EXTI_Line0);;
	OSMboxPost(pMbox, (void*)(SND_MIN_DLY + SND_DELTA_DLY * 0));
	OSIntExit();
}
void EXTI1_IrqHandler(void) {
	OSIntEnter();
	EXTI_ClearITPendingBit(EXTI_Line1);
	OSMboxPost(pMbox, (void*)(SND_MIN_DLY + SND_DELTA_DLY * 1));
	OSIntExit();
}
