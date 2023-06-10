//(АПН) -- пометка, означающая, что этот фрагмент кода был в оригигинальной лабе А. П. Новицкого

#include "includes.h"
#include <stm32f10x.h>

//макроподстановки:
//приоритеты определены в app_cfg.h
//#define APP_TASK_START_PRIO 		5 	//Приоритет стартовой задачи
//#define APP_TASK_LED_PRIO				7		//Приоритет задачи мигания светодиодом
//#define APP_TASK_KEY_POLL_PRIO 	9 	//Приоритет задачи опроса клавиатуры
//#define APP_TASK_SND_PRIO 			8		//

#define LED_BLINK_DLY 100 //Для мигания светодиодом
#define KEY_POLL_DLY 10	//Для поллинга клавиатуры

#define SND_MIN_DLY		3
#define SND_DELTA_DLY 2

//Объявление функций, на основе которых будут созданы задачи:
static void App_TaskStart (void *p_arg);
static void App_TaskLed (void *p_arg);
static void App_TaskKeyPoll (void *p_arg);
static void App_TaskSnd (void *p_arg);

//Стеки задач:
OS_STK App_TaskStartStk[APP_TASK_STK_SIZE]; //APP_TASK_STK_SIZE определен в app_cfg.h
OS_STK App_TaskLedStk[APP_TASK_STK_SIZE];
OS_STK App_TaskKeyPollStk[APP_TASK_STK_SIZE];
OS_STK App_TaskSndStk[APP_TASK_STK_SIZE];

//глобальные переменные:
u8 error; //Отладочная переменная для кодов ошибок

unsigned char cTicks; //Служебная для наблюдения "тиков" (АПН)

int  main (void) {
	BSP_IntDisAll(); //Отключение всех прерываний
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
	BSP_Init(); //Включение тактирования
	OS_CPU_SysTickInit(); //Запуск SysTick и разрешение прерываний от него
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
	
	GPIO_InitTypeDef port; 
	
	GPIO_StructInit(&port);	//Настройка PA5 для LED
	port.GPIO_Pin = GPIO_Pin_5/* | GPIO_Pin_6 | GPIO_Pin_7*/;
	port.GPIO_Mode = GPIO_Mode_Out_PP;
	port.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &port);
	
	GPIO_StructInit(&port); //Выходы для клавиатуры
	port.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
	port.GPIO_Mode = GPIO_Mode_Out_OD;
	port.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &port);
	
	GPIO_StructInit(&port);	//Входы для клавиатуры
	port.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	port.GPIO_Mode = GPIO_Mode_IPU;
	port.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &port);
	
	GPIO_StructInit(&port);	//Настройка PB2 для пищалки
	port.GPIO_Pin = GPIO_Pin_0;
	port.GPIO_Mode = GPIO_Mode_Out_PP;
	port.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &port);
	
	error = OSTaskCreate((void (*)(void *)) App_TaskKeyPoll,    //Создание задачи для мигания LED
											 (void          * ) 0,
											 (OS_STK        * ) &App_TaskKeyPollStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_KEY_POLL_PRIO
											);
	if (error) {
		__NOP();
	}
	
	error = OSTaskCreate((void (*)(void *)) App_TaskLed,    //Создание задачи для опроса клавиатуры
											 (void          * ) 0,
											 (OS_STK        * ) &App_TaskLedStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_LED_PRIO
											);
	if (error) {
		__NOP();
	}
	
	error = OSTaskCreate((void (*)(void *)) App_TaskSnd,    //Создание задачи для пищалки
											 (void          * ) 0,
											 (OS_STK        * ) &App_TaskSndStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_SND_PRIO
											);
	if (error) {
		__NOP();
	}
	
	OSTaskSuspend(APP_TASK_SND_PRIO);
	
	OSTaskDel(APP_TASK_START_PRIO); //Стартовая задача больше не требуется
	
	while(DEF_TRUE) {
	
	}
}

static void App_TaskLed (void *p_arg) {
	while(DEF_TRUE) {
		GPIO_SetBits(GPIOA, GPIO_Pin_5);
		OSTimeDly(LED_BLINK_DLY);
		GPIO_ResetBits(GPIOA, GPIO_Pin_5);
//		GPIOA->ODR ^= 1 << 5;
		OSTimeDly(OS_TICKS_PER_SEC); //макрос OS_TICKS_PER_SEC определен в os_cfg.h
	}
}

static void App_TaskKeyPoll (void *p_arg) {
	u32 sndDly;
	u8 i;
	GPIO_SetBits(GPIOC, GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12);
	while(DEF_TRUE) {
		for (i = 0; i < 3; i++) {
			GPIOC->ODR &= ~(1 << (i + 10)); 
			if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0) == RESET) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * i;
				OSTaskResume(APP_TASK_SND_PRIO);
				OSTimeDly(sndDly);
				OSTaskResume(APP_TASK_SND_PRIO);
				OSTimeDly(SND_MIN_DLY);
			}
			if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1) == RESET) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 2 + SND_DELTA_DLY * i;
				OSTaskResume(APP_TASK_SND_PRIO);
				OSTimeDly(sndDly);
				OSTaskResume(APP_TASK_SND_PRIO);
				OSTimeDly(SND_MIN_DLY);
			}
			if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2) == RESET) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 5 + SND_DELTA_DLY * i;
				OSTaskResume(APP_TASK_SND_PRIO);
				OSTimeDly(sndDly);
				OSTaskResume(APP_TASK_SND_PRIO);
				OSTimeDly(SND_MIN_DLY);
			}
			if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3) == RESET) {
				sndDly = SND_MIN_DLY + SND_DELTA_DLY * 8 + SND_DELTA_DLY * i;
				OSTaskResume(APP_TASK_SND_PRIO);
				OSTimeDly(sndDly);
				OSTaskResume(APP_TASK_SND_PRIO);
				OSTimeDly(SND_MIN_DLY);
			}
			GPIOC->ODR |= (1 << (i + 10));
		}
		OSTimeDly(KEY_POLL_DLY);
	}
}

static void App_TaskSnd (void *p_arg) {
	while(DEF_TRUE) {
//		GPIO_SetBits(GPIOB, GPIO_Pin_0);
//		OSTimeDly(sndDly);
//		GPIO_ResetBits(GPIOB, GPIO_Pin_0);
		GPIOB->ODR ^= 1;	
		OSTaskSuspend(APP_TASK_SND_PRIO);
	}
}

//  !!!  Функции ???Hook() при необходимости можно раскомментировать и наполнить действиями
//       он должен здесь быть. (АПН)

void  App_TaskCreateHook (OS_TCB *ptcb) {}
void  App_TaskDelHook (OS_TCB *ptcb) {}
void  App_TaskIdleHook (void) {}
void  App_TaskStatHook (void) {}
void  App_TaskSwHook (void) {}
void  App_TCBInitHook (OS_TCB *ptcb) {}
void  App_TimeTickHook (void) {}
	
//static void App_TaskKeyPoll (void *p_arg) {
//	u32 sndDly;
//	GPIO_SetBits(GPIOC, GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12);
//	while(DEF_TRUE) {
//		GPIO_SetBits(GPIOC, GPIO_Pin_12);
//		GPIO_ResetBits(GPIOC, GPIO_Pin_10);
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3) == RESET) {
//			sndDly = SND_MIN_DLY + SND_DELTA_DLY * 0;
//			OSTaskResume(APP_TASK_SND_PRIO);
//			OSTimeDly(sndDly);
//			OSTaskResume(APP_TASK_SND_PRIO);
//		}
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2) == RESET) {
//			sndDly = SND_MIN_DLY + SND_DELTA_DLY * 3;
//			OSTaskResume(APP_TASK_SND_PRIO);
//			OSTimeDly(sndDly);
//			OSTaskResume(APP_TASK_SND_PRIO);
//		}
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1) == RESET) {
//			sndDly = SND_MIN_DLY + SND_DELTA_DLY * 6;
//			OSTaskResume(APP_TASK_SND_PRIO);
//			OSTimeDly(sndDly);
//			OSTaskResume(APP_TASK_SND_PRIO);
//		}
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0) == RESET) {
//			sndDly = SND_MIN_DLY + SND_DELTA_DLY * 9;
//			OSTaskResume(APP_TASK_SND_PRIO);
//			OSTimeDly(sndDly);
//			OSTaskResume(APP_TASK_SND_PRIO);
//		}
//		
//		GPIO_SetBits(GPIOC, GPIO_Pin_10);
//		GPIO_ResetBits(GPIOC, GPIO_Pin_11);
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3) == RESET) {
//			sndDly = SND_MIN_DLY + SND_DELTA_DLY * 1;
//			OSTaskResume(APP_TASK_SND_PRIO);
//			OSTimeDly(sndDly);
//			OSTaskResume(APP_TASK_SND_PRIO);
//		}
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2) == RESET) {
//			sndDly = SND_MIN_DLY + SND_DELTA_DLY * 4;
//			OSTaskResume(APP_TASK_SND_PRIO);
//			OSTimeDly(sndDly);
//			OSTaskResume(APP_TASK_SND_PRIO);
//		}
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1) == RESET) {
//			sndDly = SND_MIN_DLY + SND_DELTA_DLY * 7;
//			OSTaskResume(APP_TASK_SND_PRIO);
//			OSTimeDly(sndDly);
//			OSTaskResume(APP_TASK_SND_PRIO);
//		}
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0) == RESET) {
//			sndDly = SND_MIN_DLY + SND_DELTA_DLY * 10;
//			OSTaskResume(APP_TASK_SND_PRIO);
//			OSTimeDly(sndDly);
//			OSTaskResume(APP_TASK_SND_PRIO);
//		}
//			
//		GPIO_SetBits(GPIOC, GPIO_Pin_11);
//		GPIO_ResetBits(GPIOC, GPIO_Pin_12);
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3) == RESET) {
//			sndDly = SND_MIN_DLY + SND_DELTA_DLY * 2;
//			OSTaskResume(APP_TASK_SND_PRIO);
//			OSTimeDly(sndDly);
//			OSTaskResume(APP_TASK_SND_PRIO);
//		}
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2) == RESET) {
//			sndDly = SND_MIN_DLY + SND_DELTA_DLY * 5;
//			OSTaskResume(APP_TASK_SND_PRIO);
//			OSTimeDly(sndDly);
//			OSTaskResume(APP_TASK_SND_PRIO);
//		}
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1) == RESET) {
//			sndDly = SND_MIN_DLY + SND_DELTA_DLY * 8;
//			OSTaskResume(APP_TASK_SND_PRIO);
//			OSTimeDly(sndDly);
//			OSTaskResume(APP_TASK_SND_PRIO);
//		}
//		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0) == RESET) {
//			sndDly = SND_MIN_DLY + SND_DELTA_DLY * 11;
//			OSTaskResume(APP_TASK_SND_PRIO);
//			OSTimeDly(sndDly);
//			OSTaskResume(APP_TASK_SND_PRIO);
//		}
//	
//		OSTimeDly(KEY_POLL_DLY);
//	}
//}
