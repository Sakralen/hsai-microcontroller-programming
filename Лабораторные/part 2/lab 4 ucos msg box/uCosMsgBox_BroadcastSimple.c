#define BROADCAST_SIMPLE_CFG

#include "includes.h"
#include <stm32f10x.h>

s8* Dec_Convert(s8* buf, s32 value);
u16 Write1_Poll(s8* ptr);
u16 Write1_Poll_Uart2(s8* ptr);

static void App_TaskStart (void *p_arg);
static void App_TaskSender (void *p_arg);
static void App_TaskReciever1 (void *p_arg);
static void App_TaskReciever2 (void *p_arg);

OS_STK App_TaskStartStk[APP_TASK_STK_SIZE];
OS_STK App_TaskSenderStk[APP_TASK_STK_SIZE];
OS_STK App_TaskReciever1chStk[APP_TASK_STK_SIZE];
OS_STK App_TaskReciever2chStk[APP_TASK_STK_SIZE];

u16 sendOsc;

OS_EVENT *mbox;

u8 err;
unsigned char cTicks;

int main(void) {
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

void App_TaskStart(void* p_arg) {
	BSP_Init();
	OS_CPU_SysTickInit();
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA /*| RCC_APB2Periph_GPIOC*/ | RCC_APB2Periph_ADC1 | RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	
	GPIO_InitTypeDef port;
	
	GPIO_StructInit(&port);
	port.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_9;
	port.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &port);
	
	USART_InitTypeDef usart;
  usart.USART_BaudRate = 9600;
	usart.USART_WordLength = USART_WordLength_8b;
  usart.USART_StopBits = USART_StopBits_1;
  usart.USART_Parity = USART_Parity_No;
  usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  usart.USART_Mode = USART_Mode_Tx;
  USART_Init(USART1, &usart);   
	USART_Init(USART2, &usart);
		
  USART_Cmd(USART1, ENABLE);
  USART_Cmd(USART2, ENABLE);	
		
	if (0 == (mbox = OSMboxCreate(0))) {
		__NOP();
	}
	
	OSTaskCreate((void (*)(void *)) App_TaskSender,
											 (void          * ) 0,
											 (OS_STK        * ) &App_TaskSenderStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_SENDER_PRIO
							);
	OSTaskCreate((void (*)(void *)) App_TaskReciever1,
										 (void          * ) 0,
										 (OS_STK        * ) &App_TaskReciever1chStk[APP_TASK_STK_SIZE - 1],
										 (INT8U           ) APP_TASK_RECIEVER1_PRIO
							);										 									 
	OSTaskCreate((void (*)(void *)) App_TaskReciever2,
											 (void          * ) 0,
											 (OS_STK        * ) &App_TaskReciever2chStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_RECIEVER2_PRIO
							);
											 
	OSTaskDel(APP_TASK_START_PRIO);
	
	while(DEF_TRUE) {
	
	}
}
	
static void App_TaskSender (void *p_arg) {
	s8* msg = (s8*)"A message";
	while (DEF_TRUE) {
	if (OS_MBOX_FULL == OSMboxPostOpt(mbox, msg, OS_POST_OPT_BROADCAST)) {
	//if (OS_MBOX_FULL == OSMboxPostOpt(mbox,	msg, OS_POST_OPT_NONE)) { 				
       __NOP();
		}
    OSTaskSuspend(OS_PRIO_SELF);      
  }	
}

static void App_TaskReciever1 (void *p_arg) {
	s8* msg;	
	while (DEF_TRUE) {
		msg = (OSMboxPend(mbox, 0, &err));		
    Write1_Poll((s8*)msg);
	}
}

static void App_TaskReciever2 (void *p_arg) {
	s8* msg;
	while (DEF_TRUE) {
		msg = (OSMboxPend(mbox, 0, &err));		
    Write1_Poll_Uart2((s8*)msg);
	}
}

//----------------- Вспомогательные функции --------------------------------
//  Преобразование 32-битовой величины со знаком в 10-чную ASCII-строку
#define FALSE 0
#define TRUE !FALSE

s8* Dec_Convert(s8* buf, s32 value) 
{
	int divider = 1000000000;     
	unsigned char bNZflag=FALSE, minus=FALSE;		//  Флаги левых нулей и минуса
	unsigned char current_digit;

	if (value < 0) {		//    Если число value отрицательное 
		minus=TRUE;
		value = -value;
	}
	while (divider) {
		current_digit = value / divider;
		if (current_digit || bNZflag) { //  Как только получили ненулевую цифру,
		  	if (minus) { 	      //  Если число отрицательное, то поставим -
		    	buf--;
				*buf++ = '-';
				minus=FALSE; 
		  	} 
			value %= divider;
			*buf++ = current_digit + '0';
			bNZflag = TRUE;				// это значит, что левые нули закончились
		} else {  			//  Вместо левых нулей - пробелы, чтобы выровнять вправо
		    *buf++ = ' ';
		}
		divider /= 10;
    }
	if (!bNZflag)
		*buf++ = '0';
	*buf = 0;				//  Это нуль-терминатор (признак окончания строки)
  return buf;
}

//  Функция передачи строки через USART1 с поллингом без проверок параметров
u16 Write1_Poll(s8* ptr) 
{
  int32_t len = 0;
  while (*ptr) {
    USART_SendData(USART1, *ptr);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) { }
    ptr++;  
		len++;
//    while (GetChar1() != ' ') { }    //  Ожидание приема пробела
  }
  return len;
}

u16 Write1_Poll_Uart2(s8* ptr) 
{
  int32_t len = 0;
  while (*ptr) {
    USART_SendData(USART2, *ptr);
    while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) { }
    ptr++;  
		len++;
//    while (GetChar1() != ' ') { }    //  Ожидание приема пробела
  }
  return len;
}
