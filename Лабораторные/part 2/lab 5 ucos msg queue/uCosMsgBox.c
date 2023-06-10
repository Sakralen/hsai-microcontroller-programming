#define SEM_CFG

#include "includes.h"
#include <stm32f10x.h>

//#define USE_MBOX_MUTUAL

#define MEAS_OSC 100
#define SEND_OSC 30

#define TO_MILIVOLTS(x) x * 3300 / 4096

s8* Dec_Convert(s8* buf, s32 value);
u16 Write1_Poll(s8* ptr);

static void App_TaskStart (void *p_arg);
static void App_TaskMeas (void *p_arg);
static void App_TaskSend (void *p_arg);

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
	s8* chStr;				//Строка с номером канала
	OS_EVENT *mbox;
} MeasTaskArgs;

MeasTaskArgs args1ch, args2ch, args3ch;
u16 sendOsc;

OS_EVENT *uartSem, *mbox1, *mbox2, *mbox3;

//#ifdef USE_MBOX_MUTUAL
//OS_EVENT* mBoxMutual;
//#endif

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
		
	if (0 == (uartSem = OSSemCreate(1))) {
		__NOP();
	}
	
	if (0 == (mbox1 = OSMboxCreate(0))) {
		__NOP();
	}
	if (0 == (mbox2 = OSMboxCreate(0))) {
		__NOP();
	}
	if (0 == (mbox3 = OSMboxCreate(0))) {
		__NOP();
	}
	
	args1ch.adcChannel = ADC_Channel_0;
	args1ch.sampleTime = ADC_SampleTime_41Cycles5;	
	args1ch.seriesLength = 13;
	args1ch.discrPeriod = 123;
	args1ch.chStr = (s8*)"Channel 1 result: ";
	args1ch.mbox = mbox1;
	
	args2ch.adcChannel = ADC_Channel_1;
	args2ch.sampleTime = ADC_SampleTime_28Cycles5;	
	args2ch.seriesLength = 10;
	args2ch.discrPeriod = 55;	
	args2ch.chStr = (s8*)"Channel 2 result: ";	
	args2ch.mbox = mbox2;
	
	args3ch.adcChannel = ADC_Channel_2;
	args3ch.sampleTime = ADC_SampleTime_41Cycles5;	
	args3ch.seriesLength = 15;
	args3ch.discrPeriod = 89;
	args3ch.chStr = (s8*)"Channel 3 result: ";
	args3ch.mbox = mbox3;
	
	OSTaskCreate((void (*)(void *)) App_TaskMeas,
											 (void          * ) &args1ch,
											 (OS_STK        * ) &App_TaskMeas1chStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_MEAS1_PRIO
							);
	OSTaskCreate((void (*)(void *)) App_TaskSend,
											 (void          * ) &args1ch,
											 (OS_STK        * ) &App_TaskSend1chStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_SEND1_PRIO
							);
	OSTaskCreate((void (*)(void *)) App_TaskMeas,
											 (void          * ) &args2ch,
											 (OS_STK        * ) &App_TaskMeas2chStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_MEAS2_PRIO
							);
	OSTaskCreate((void (*)(void *)) App_TaskSend,
											 (void          * ) &args2ch,
											 (OS_STK        * ) &App_TaskSend2chStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_SEND2_PRIO
							);
	OSTaskCreate((void (*)(void *)) App_TaskMeas,
											 (void          * ) &args3ch,
											 (OS_STK        * ) &App_TaskMeas3chStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_MEAS3_PRIO
							);
	OSTaskCreate((void (*)(void *)) App_TaskSend,
											 (void          * ) &args3ch,
											 (OS_STK        * ) &App_TaskSend3chStk[APP_TASK_STK_SIZE - 1],
											 (INT8U           ) APP_TASK_SEND3_PRIO
							);
											 
	OSTaskDel(APP_TASK_START_PRIO);
	
	while(DEF_TRUE) {
	
	}
}
	
static void App_TaskMeas (void *p_arg) {
	OS_CPU_SR cpu_sr = 0;
	MeasTaskArgs* args = (MeasTaskArgs*)p_arg;
	u16 partialSum; 	//Частичная сумма
	u8 curSeriesCnt; 	//Счетчик серии
	u32 resultOut;
	
	while (DEF_TRUE) {
		OS_ENTER_CRITICAL();
		ADC_RegularChannelConfig(ADC1, args->adcChannel, 1, args->sampleTime);
		ADC_Cmd(ADC1, ENABLE);
		while(RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)) {}
		OS_EXIT_CRITICAL();
			
		partialSum += ADC_GetConversionValue(ADC1);
		if(args->seriesLength == ++(curSeriesCnt)) {
			resultOut = TO_MILIVOLTS(partialSum) / args->seriesLength;
			curSeriesCnt = 0;
			partialSum = 0;
			if (OS_MBOX_FULL == OSMboxPost(args->mbox, &resultOut)) { 
        __NOP();
			}
		}			
    OSTimeDly(args->discrPeriod);      
  }	
}

static void App_TaskSend (void *p_arg) {
	MeasTaskArgs* args = (MeasTaskArgs*)p_arg;
	u8 buffer[20];		//Буфер для передачи строки
	u32 resultIn;
	
	while (DEF_TRUE) {
		resultIn = *((u32*)OSMboxPend(args->mbox, 0, &err));
		sendOsc += SEND_OSC;
		Dec_Convert((s8*)(buffer), (s32)(resultIn));
		OSSemPend(uartSem, 0, &err);
		Write1_Poll((s8*)args->chStr);
    Write1_Poll((s8*)&(buffer)[5]);
    Write1_Poll((s8*)" mV\n");
		sendOsc -= SEND_OSC;
		OSSemPost(uartSem);
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
