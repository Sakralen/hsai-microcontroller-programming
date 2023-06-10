#include "includes.h"
#include <stm32f10x.h>

//#define OS_MAX_TASKS              7u

//#define APP_TASK_START_PRIO 							4

//#define  APP_TASK_MEAS1_PRIO             	6      
//#define  APP_TASK_SEND1_PRIO             	10

//#define  APP_TASK_MEAS2_PRIO             	7      
//#define  APP_TASK_SEND2_PRIO            	11

//#define  APP_TASK_MEAS3_PRIO             	8      
//#define  APP_TASK_SEND3_PRIO           	 	12

#define MEAS_OSC 100

#define TO_MILIVOLTS(x) x * 3300 / 4096

#define USE_SEMAPHORE

static void App_TaskStart (void *p_arg);

static void App_TaskMeas (void *p_arg);
static void App_TaskSend (void *p_arg);

s8* Dec_Convert(s8* buf, s32 value);
u16 Write1_Poll(s8* ptr) ;

OS_STK App_TaskStartStk[APP_TASK_STK_SIZE];

OS_STK App_TaskMeas1chStk[APP_TASK_STK_SIZE];
OS_STK App_TaskSend1chStk[APP_TASK_STK_SIZE];

OS_STK App_TaskMeas2chStk[APP_TASK_STK_SIZE];
OS_STK App_TaskSend2chStk[APP_TASK_STK_SIZE];

OS_STK App_TaskMeas3chStk[APP_TASK_STK_SIZE];
OS_STK App_TaskSend3chStk[APP_TASK_STK_SIZE];

typedef struct {		//Аргумент задачи измерения
	u8 adcChannel; 		//Канал АЦП
	u8 sampleTime;		//Время выборки
	u16 discrPeriod; 	//Период дискретизации 
	u16 seriesLength; //Длина серии для усреднения
	u16 partialSum; 	//Частичная сумма
	u32 result;				//Результат усреднения
	u8 curSeriesCnt; 	//Счетчик серии
	u8 buffer[20];		//Буфер для передачи строки
	u8 sendTaskPrio;	//Приоритет соотв. задачи для передачи результата
	s8* chStr;				//Строка с номером канала
	u8 sendOsc;
	u8 sendOscStep;
} MeasTaskArgs;			//Некоторые аргументы можно сделать локальными переменными задачи

MeasTaskArgs args1ch, args2ch, args3ch;

u16 sendOscGlobal;

#ifdef USE_SEMAPHORE
OS_EVENT* uartSem;
#endif
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
	
	GPIO_InitTypeDef port;
	
	GPIO_StructInit(&port);
	port.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
	port.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &port);
	
	GPIO_StructInit(&port);
	port.GPIO_Pin = GPIO_Pin_9;
	port.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &port);
	
	ADC_InitTypeDef adc;
	
  adc.ADC_Mode = ADC_Mode_Independent;
  adc.ADC_ScanConvMode = DISABLE;
  adc.ADC_ContinuousConvMode = DISABLE;
  adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  adc.ADC_DataAlign = ADC_DataAlign_Right;
  adc.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &adc);
    
  ADC_Cmd(ADC1, ENABLE);

  ADC_ResetCalibration(ADC1);
  while(ADC_GetResetCalibrationStatus(ADC1)) {}
  ADC_StartCalibration(ADC1);
  while(ADC_GetCalibrationStatus(ADC1)){}
		
	USART_InitTypeDef usart;
  usart.USART_BaudRate = 9600;
	usart.USART_WordLength = USART_WordLength_8b;
  usart.USART_StopBits = USART_StopBits_1;
  usart.USART_Parity = USART_Parity_No;
  usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  usart.USART_Mode = USART_Mode_Tx;
  USART_Init(USART1, &usart);   

  USART_Cmd(USART1, ENABLE);
		
	args1ch.adcChannel = ADC_Channel_0;
	args1ch.sampleTime = ADC_SampleTime_41Cycles5;	
	args1ch.seriesLength = 13;
	args1ch.discrPeriod = 7;
	args1ch.partialSum = 0;
	args1ch.result = 0;
	args1ch.curSeriesCnt = 0;	
	args1ch.sendTaskPrio = APP_TASK_SEND1_PRIO;
	args1ch.chStr = (s8*)"Channel 1 result: ";
	args1ch.sendOsc = 0;
	args1ch.sendOscStep = 20;
	
	args2ch.adcChannel = ADC_Channel_1;
	args2ch.sampleTime = ADC_SampleTime_28Cycles5;	
	args2ch.seriesLength = 10;
	args2ch.discrPeriod = 11;
	args2ch.partialSum = 0;
	args2ch.result = 0;
	args2ch.curSeriesCnt = 0;	
	args2ch.sendTaskPrio = APP_TASK_SEND2_PRIO;
	args2ch.chStr = (s8*)"Channel 2 result: ";	
	args2ch.sendOsc = 0;
	args2ch.sendOscStep = 60;

	args3ch.adcChannel = ADC_Channel_2;
	args3ch.sampleTime = ADC_SampleTime_41Cycles5;	
	args3ch.seriesLength = 15;
	args3ch.discrPeriod = 13;
	args3ch.partialSum = 0;
	args3ch.result = 0;
	args3ch.curSeriesCnt = 0;	
	args3ch.sendTaskPrio = APP_TASK_SEND3_PRIO;
	args3ch.chStr = (s8*)"Channel 3 result: ";
	args3ch.sendOsc = 0;
	args3ch.sendOscStep = 120;	
	
#ifdef USE_SEMAPHORE
	if (0 == (uartSem = OSSemCreate(1))) {
		__NOP();
	}
#endif
	
	err = OSTaskCreate((void (*)(void *)) App_TaskMeas,
											 (void          * ) &args1ch,
											 (OS_STK        * ) &App_TaskMeas1chStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_MEAS1_PRIO
											);
	if (err) {
		__NOP();
	}
	
	err = OSTaskCreate((void (*)(void *)) App_TaskSend,
											 (void          * ) &args1ch,
											 (OS_STK        * ) &App_TaskSend1chStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_SEND1_PRIO
											);
	if (err) {
		__NOP();
	}

	err = OSTaskCreate((void (*)(void *)) App_TaskMeas,
											 (void          * ) &args2ch,
											 (OS_STK        * ) &App_TaskMeas2chStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_MEAS2_PRIO
											);
	if (err) {
		__NOP();
	}
	
	err = OSTaskCreate((void (*)(void *)) App_TaskSend,
											 (void          * ) &args2ch,
											 (OS_STK        * ) &App_TaskSend2chStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_SEND2_PRIO
											);
	if (err) {
		__NOP();
	}
	
	err = OSTaskCreate((void (*)(void *)) App_TaskMeas,
											 (void          * ) &args3ch,
											 (OS_STK        * ) &App_TaskMeas3chStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_MEAS3_PRIO
											);
	if (err) {
		__NOP();
	}
	
	err = OSTaskCreate((void (*)(void *)) App_TaskSend,
											 (void          * ) &args3ch,
											 (OS_STK        * ) &App_TaskSend3chStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_SEND3_PRIO
											);
	if (err) {
		__NOP();
	}
	
	OSTaskDel(APP_TASK_START_PRIO);
	
	while(DEF_TRUE) {
	
	}
}
	
static void App_TaskMeas (void *p_arg) {
	MeasTaskArgs* args = (MeasTaskArgs*)p_arg;
	OS_CPU_SR cpu_sr = 0;
	
	while (DEF_TRUE) {
		OS_ENTER_CRITICAL();
		args->result += MEAS_OSC;
		ADC_RegularChannelConfig(ADC1, args->adcChannel, 1, args->sampleTime);
		ADC_Cmd(ADC1, ENABLE);
		while(RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)) {}
		args->result -= MEAS_OSC;
		OS_EXIT_CRITICAL();
			
		args->partialSum += ADC_GetConversionValue(ADC1);
		if(args->seriesLength == ++(args->curSeriesCnt)) {
			args->curSeriesCnt = 0;
			args->result = TO_MILIVOLTS(args->partialSum) / args->seriesLength;
			args->partialSum = 0;
			OSTaskResume(args->sendTaskPrio);
		}			
		 
    OSTimeDly(args->discrPeriod);      
  }	
}

static void App_TaskSend (void *p_arg) {
	MeasTaskArgs* args = (MeasTaskArgs*)p_arg;
	while (DEF_TRUE) {
		OSTaskSuspend(OS_PRIO_SELF);
		
		args->sendOsc += args->sendOscStep;
		sendOscGlobal = args1ch.sendOsc + args2ch.sendOsc + args3ch.sendOsc;
		
		Dec_Convert((s8*)(args->buffer), (s32)(args->result));
#ifdef USE_SEMAPHORE
		OSSemPend(uartSem, 0, &err);
#endif
		Write1_Poll((s8*)args->chStr);
    Write1_Poll((s8*)&(args->buffer)[5]);
    Write1_Poll((s8*)" mV\n");
		args->sendOsc = 0;
		sendOscGlobal = args1ch.sendOsc + args2ch.sendOsc + args3ch.sendOsc;
#ifdef USE_SEMAPHORE
		OSSemPost(uartSem);
#endif
	}
}

//#define DUMMY_DELAY 1000000
//static void App_Dummy (void *p_arg) { 
//	u8 dummyOsc;
//	u32 i;
//	while (DEF_TRUE) {
//		dummyOsc = 100;
//		for (i = 0; i < DUMMY_DELAY; i++) {}
//		dummyOsc = 0;
//		OSTaskSuspend(OS_PRIO_SELF);
//	}
//}

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

