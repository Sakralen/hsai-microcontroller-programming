
/******************************************************************************************************\
*  Программа двухканального измерения под ОС РВ uC/OS-II v.2.86   
* Программа есть развитие uCOS_ADC_UART. 
*
*  Filename:    uCOS_ADC_UART_2Ch.c
* Программа  выполняет то же, что и предыдущая (в конфигурации uCOS_ADC_UART), 
* но для двух входных аналоговых величин.
*
\******************************************************************************************************/

#include "includes.h"
#include <stm32f10x.h>

#define   IZM   400

  //  На базе этих массивов будут (при работе системной функции OSTaskCreate(...)) созданы стеки Задач 
OS_STK         App_TaskStartStk[64];

OS_STK         App_TaskMeas1Stk[64];
OS_STK         App_TaskSend1Stk[64];

OS_STK         App_TaskMeas2Stk[APP_TASK_STK_SIZE];
OS_STK         App_TaskSend2Stk[APP_TASK_STK_SIZE];

  //  Функция для контроля размера стеков при отладке. Ей надо давать в качестве параметра
  // имя массива, на базе которой организован стек Задачи, например, вот так:
  // Var=FreeStkSpace(App_TaskStartStk);  
  //   В переменную Var вернется число "чистых" элементов в стеке Задачи TaskStart
u16 FreeStkSpace(OS_STK * x) 	// 	Функция должна быть атомарной ??
{ OS_STK * pS  = x;						//	Указатель на стек (локально 4 байта)
  OS_CPU_SR cpu_sr = 0; 			//	Переменная для сост.CPU (локально 4 байта)
  u16 StkCtr   = 0;						//	Счетчик свободных (локально	2 байта, всего 3 эл.в стеке)
    OS_ENTER_CRITICAL();  //  проверка стека должна быть атомарной ???
    while (*pS==0) { StkCtr++; pS++;}
    OS_EXIT_CRITICAL();
    return StkCtr;
}

//  Объявления функций, на базе которых в программе будут созданы Задачи
//  Определения этих функций смотрите далее в этом файле 
static  void  App_TaskStart    (void        *p_arg);
static  void  App_TaskMeas1    (void        *p_arg);
static  void  App_TaskSend1    (void        *p_arg);
static  void  App_TaskMeas2    (void        *p_arg);
static  void  App_TaskSend2    (void        *p_arg);

u8 err;               //  Для кода ошибки
unsigned char cTicks;    //  Служебная для наблюдения "тиков"
u8 cADC1=100;
u8 ucSend=0, ucSend1=0, ucSend2=0;  //  Длит.передачи в UART

u16   usRez1  =0;     //  Переменная для результата1 
u8    cBuf1[20];      //  Буфер для строки результата1

u32   usRez2  =0;     //  Переменная для результата2
u8    cBuf2[20];      //  Буфер для строки результата2

s8* Dec_Convert(s8* buf, s32 value);
u16 Write1_Poll(s8* ptr);

/*******************************************************************************************************\
*                                                main()
\*******************************************************************************************************/
int  main (void) {
//    CPU_INT08U  os_err;
  BSP_IntDisAll();       // Disable all ints until we are ready to accept them.

  OSInit();              // Initialize "uC/OS-II, The Real-Time Kernel"
  OSTaskCreate((void (*)(void *)) App_TaskStart,     // Create the start task.                               */
               (void          * ) 0,
               (OS_STK        * )&App_TaskStartStk[APP_TASK_START_STK_SIZE - 1],
               (INT8U           ) APP_TASK_START_PRIO     //  Уровень приоритета 
              );
  OSStart();              // Start multitasking (i.e. give control to uC/OS-II).
        //  Возврата из функции OSStart() не произойдет никогда.
  return (0);   //  Сюда управление не попадет никогда
}

/*******************************************************************************************************\
*        App_TaskStart()
* Description : Стартовая (начальная) Задача. Она должна получить управление первой. Для этого ей
*   следует присваивать наивысший приоритет (в случае, если в main() вы создадите несколько Задач
*   В стартовой Задаче (не раньше!!!) следует запустить системный таймер и разрешить от него прерывание
* 
*   Стартовая Задача - самое подходящее место для настроек периферийных подсистем 
   и для создания Задач прикладной программы
\*******************************************************************************************************/

static  void  App_TaskStart (void *p_arg) {
//  u16 volatile usStkFree=APP_TASK_START_STK_SIZE;
//  usStkFree=FreeStkSpace(App_TaskStartStk);   //  Проверка переполнения стека Задачи

  BSP_Init();   //  Фактически - только настройка RCC - 72 (или 24) МГц от ФАПЧ 
  OS_CPU_SysTickInit();  // Запуск системного таймера и прерывания от его переполнения
        //  После этого SysTick работает, и многозадачность функционирует полностью
//------------------------------------------------------------------------------------

  //  Разрешить тактирование нужных подсистем
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA |  //  Порт PA - два входа аналоговых
                          RCC_APB2Periph_ADC1 |   //  АЦП
                          RCC_APB2Periph_USART1,  //  UART
                            ENABLE);   

  //  Настроить выводы порта PA1 PA2 как аналоговые входы
    GPIO_InitTypeDef gpioA;     //  Настроечная структура для функции  GPIO_Init(...)
  gpioA.GPIO_Pin    = GPIO_Pin_1 | GPIO_Pin_2;    //  для подключения датчиков
  gpioA.GPIO_Mode   = GPIO_Mode_AIN;      // Выводы порта - в режиме аналоговых входов   
    GPIO_Init(GPIOA, &gpioA);   //  Вызов настроечной функции для порта (см.справку по SPL)
  
  //  Настроить выводы PA9, PA10 - как выход Tx И вход Rx USART1
  gpioA.GPIO_Pin = GPIO_Pin_9|GPIO_Pin_10;         //  Выходы Tx|Rx USART1
  gpioA.GPIO_Mode = GPIO_Mode_AF_PP;
  gpioA.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpioA);

    //  Настройка АЦП общая
    ADC_InitTypeDef ADC_InitStructure;
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;    //  Если два или более АЦП (не использ)
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;         //  Режима сканирования нет
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;   //  Непрер.режима нет
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; //  Внеш.запуск ОТКЛ
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;  //  Рез.измер. выровнен вправо
  ADC_InitStructure.ADC_NbrOfChannel = 1;               //  Число каналов 1
    ADC_Init(ADC1, &ADC_InitStructure);           //  Настройка
    
  ADC_Cmd(ADC1, ENABLE);      //  Разрешение работы АЦП

    //  Калибровка АЦП
  ADC_ResetCalibration(ADC1); //  Сброс встроенной калибровки
  while(ADC_GetResetCalibrationStatus(ADC1)) {} //  Ожидание конца сброса
  ADC_StartCalibration(ADC1); //  Запуск калибровки
  while(ADC_GetCalibrationStatus(ADC1)){}   //  Ожидание конца калибровки

    //  Настройка UART
    USART_InitTypeDef USART_InitStructure;      //  Задание настроечных параметров для USART1
  USART_InitStructure.USART_BaudRate = 38400;   //  Скорость 38 кбит/с 
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx; //  Нужeн Tx 
    USART_Init(USART1, &USART_InitStructure);   

  USART_Cmd(USART1, ENABLE);    //  Включили UART
 

//  Здесь самое подходящее место для создания пользовательских Задач, 
// объектов синхронизации-коммуникации и т.п.

//  Помните, что если у создаваемой Задачи приоритет выше, чем у стартовой Задачи, то
// вновь созданная Задача будет немедленно запущена на исполнение, а работа стартовой
// Задачи будет приостановлена

  OSTaskCreate((void (*)(void *)) App_TaskMeas1,    //  Создадим Задачу для измерения1
               (void          * ) 0,
               (OS_STK        * ) &App_TaskMeas1Stk[APP_TASK_STK_SIZE - 1],  //  Стек Задачи
               (INT8U           ) APP_TASK_MEAS1_PRIO   // Уровень приоритета для этой Задачи =5
              );

  OSTaskCreate((void (*)(void *)) App_TaskSend1,        //  Создадим Задачу для передачи
               (void          * ) 0,
               (OS_STK        * ) &App_TaskSend1Stk[APP_TASK_STK_SIZE - 1],
               (INT8U           ) APP_TASK_SEND1_PRIO   // Уровень приоритета для этой Задачи =9
              );

  OSTaskCreate((void (*)(void *)) App_TaskMeas2,        //  Создадим Задачу для измерения1
               (void          * ) 0,
               (OS_STK        * ) &App_TaskMeas2Stk[APP_TASK_STK_SIZE - 1],  //  Стек Задачи
               (INT8U           ) APP_TASK_MEAS2_PRIO   // Уровень приоритета для этой Задачи =6
              );

  if((err=                    //  При создании последней Задачи проверка успешности создания
  OSTaskCreate((void (*)(void *)) App_TaskSend2,        //  Создадим Задачу для опроса клавиатуры
               (void          * ) 0,
               (OS_STK        * ) &App_TaskSend2Stk[APP_TASK_STK_SIZE - 1],
               (INT8U           ) APP_TASK_SEND2_PRIO   // Уровень приоритета для этой Задачи =11
              )) !=OS_ERR_NONE)
  {
    __NOP();
  }     //  Значения кодов ошибок в файле ucos_ii.h, со строки 245

//  Уровни приоритета (целые в пределах от 0 до 254) должны быть различными для разных Задач
//  Константы уровней приоритета APP_TASK_START_PRIO, APP_TASK_MEAS1_PRIO,...  
// будут использоваться как параметры в системных сервисах,
// управляющих Задачами для указания конкретной Задачи
  
  
 //  Задача App_TaskStart больше не нужна, с ней можно поступить несколькими способами, вот они:
//  OSTaskSuspend(OS_PRIO_SELF);      //  (1)   Системный сервис переводит Задачу App_TaskStart в ожидание 
    //  (2)   Можно было бы удалить Задачу, если она больше не нужна, вот так
  OSTaskDel(APP_TASK_START_PRIO);   //  Здесь уровень приоритета (идентификатор удаляемой Задачи) указан явно
    //  (3)   Бесконечный цикл
  while (DEF_TRUE) {      //  В Задаче должен быть бесконечный цикл
    //  В этот цикл можно поместить полезный код
    //  Надо помнить, что приоритет стартовой Задачи выше приоритета всех прочих созданных Задач
    // если здесь будет "полезный код", в нем должна (где-то) выполняться приостановка данной Задачи
  }
    //  Но во всяком случае Задаче нельзя позволить завершиться выходом на закрывающую скобку:
}   // сюда попасть - недопустимо!!!


/*******************************************************************************************************\
*                                            App_TaskMeas1()
*
* Description : Задача выполняет измерения по одному каналу
*     с периодом дискретизации TDiscr1
* Уникальный уровень приоритета для этой Задачи  p_MEAS1 = 5 
* 
\*******************************************************************************************************/

//  Параметры регистрации для сигнала1 (переменные глобальные)
u16   TDiscr1 =10;    //  Период дискретизации сигнала1 (10 - 1 мс)
u16   NSer1   =13;    //  Длина серии для сигнала1
u32   uiSum1  =0;
u16   usCtr1  =0;     //  счетчик отсчетов в серии1
u8    cADC=0;

static void App_TaskMeas1(void *p_arg) {
  u16 volatile usStkFree=128;   //  Для контроля стека
  usStkFree=FreeStkSpace(App_TaskStartStk);

  while (DEF_TRUE) 
  {    //  В Задаче должен быть бесконечный цикл
//  Для отладки можно остановить созданную Задачу
//    OSTaskSuspend(OS_PRIO_SELF);    //  Можно было указать приоритет APP_TASK_MEAS1_PRIO

      //  Включить канал АЦП

    usRez1+=IZM;  //  Метка начала АЦПреобр на графике usRez1
    //  Выбор канала 1, задание времени выборки
    //  Времена выборки: 1, 7, 13, 28, 41, 55, 71, 239
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_13Cycles5);

      //  Запустить АЦП
    ADC_Cmd(ADC1, ENABLE);  //  Запустить АЦП можно и так
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)==RESET)   //  Ожидать результата 
    {  //  Ожидание результата АЦ преобразования
      __NOP();
    }
    usRez1-=IZM;        //  Метка конца АЦПреобр на графике usRez1

//    uiSum1+=ADC1->DR;
    uiSum1+=ADC_GetConversionValue(ADC1);  // и прибавить к частичной сумме
    cADC=0;
    if((usCtr1++)==NSer1)
    { usCtr1=0;
      usRez1=uiSum1*3300/4096/NSer1;
      uiSum1=0;

      //  Выполнить передачу
      OSTaskResume(APP_TASK_SEND1_PRIO);
    }
		usStkFree=FreeStkSpace(App_TaskStartStk);		//	Контроль стека Задачи
      //  Запустить тайм-аут на период дискретизации
    OSTimeDly(TDiscr1);      
  }
}

/*******************************************************************************************************\
*                                            App_TaskSend1()
*
* Description : Задача выполняет передачу отсчетов первого канала
* Уникальный уровень приоритета для этой Задачи   = 9 
* 
\*******************************************************************************************************/

static void App_TaskSend1(void *p_arg) 
{
//  u16 volatile usStkFree=128;
//  usStkFree=FreeStkSpace(App_TaskStartStk);

  while (DEF_TRUE) 
  {        //  В Задаче должен быть бесконечный цикл
    OSTaskSuspend(OS_PRIO_SELF);    //  Можно было указать приоритет APP_TASK_SEND1_PRIO
      //  Задача App_TaskSend1() будет продолжена, когда будут готовы данные для передачи
    ucSend1=100;
      ucSend=ucSend1+ucSend2;       //  Для наблюдения времени передачи
      //  Преобразовать число в строку
    Dec_Convert((s8*)cBuf1, (s32)usRez1);
    Write1_Poll((s8*)"Signal_1 = ");
    Write1_Poll((s8*)&cBuf1[5]);
    Write1_Poll((s8*)" mV\n");
    ucSend1=0;
      ucSend=ucSend1+ucSend2;       //  Для наблюдения времени передачи
    __NOP();
    OSTaskResume(APP_TASK_MEAS2_PRIO);    //  Можно было указать приоритет
  }   //  В Задаче должен быть бесконечный цикл, здесь его конец
}     //  Здесь заканчивается тело функции, на базе которой создается Задача


/*******************************************************************************************************\
*                                            App_TaskMeas2()
* Description : Задача выполняет измерения по второму каналу
*     с периодом дискретизации TDiscr2
* Уникальный уровень приоритета для этой Задачи  p_MEAS2 = 6 
\*******************************************************************************************************/

//  Параметры регистрации для сигнала2  (переменные глобальные)
u16   TDiscr2 =17;            //  Период дискретизации сигнала2 (10->1 мс)
u16   NSer2   =21;            //  Длина серии для сигнала2
u32   uiSum2  =0;
u16   usCtr2  =0;             //  счетчик отсчетов в серии2

static void App_TaskMeas2(void *p_arg) 
{
  u16 volatile usStkFree=128;     //  Для контроля стека
  usStkFree=FreeStkSpace(App_TaskStartStk);
    OSTaskSuspend(OS_PRIO_SELF);    //  Можно было указать приоритет

  while (DEF_TRUE) 
  {   //  В Задаче должен быть бесконечный цикл
      //  Включить канал АЦП

    usRez2+=IZM;              //  Метка начала АЦПреобр на графике usRez2
    //  Выбор канала 2, задание времени выборки
    //  Времена выборки: 1, 7, 13, 28, 41, 55, 71, 239
    ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_28Cycles5);
      //  Запустить АЦП
    ADC_Cmd(ADC1, ENABLE);  //  Запустить АЦП
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)==RESET)   //  Ожидать результата 
    {  //  Ожидание результата АЦ преобразования
      __NOP();
    }
    uiSum2+=ADC_GetConversionValue(ADC1);  // и прибавить к частичной сумме
    usRez2-=IZM;        //  Метка конца АЦПреобр на графике usRez2

    if((usCtr2++)==NSer2)
    { usCtr2=0;
      usRez2=uiSum2*3300/4096/NSer2;
      uiSum2=0;

      //  Выполнить передачу
      OSTaskResume(APP_TASK_SEND2_PRIO);
    }
      //  Запустить тайм-аут на период дискретизации
    OSTimeDly(TDiscr2);      
  }
}

/*******************************************************************************************************\
*                                            App_TaskSend2()
* Description : Задача выполняет передачу отсчетов второго канала
* Уникальный уровень приоритета для этой Задачи   = 11 
\*******************************************************************************************************/

static void App_TaskSend2(void *p_arg) 
{
  OS_CPU_SR cpu_sr = 0;
//  u16 volatile usStkFree=128;
//  usStkFree=FreeStkSpace(App_TaskStartStk);

  while (DEF_TRUE) 
  {                //  В Задаче должен быть бесконечный цикл
    OSTaskSuspend(OS_PRIO_SELF);    //  Можно было указать приоритет 
      //  Задача App_TaskSend1() будет продолжена, когда будут готовы данные для передачи
    __NOP();
    ucSend2=60;
      ucSend=ucSend1+ucSend2;       //  Для наблюдения времени передачи
      //  Преобразовать число в строку
    Dec_Convert((s8*)cBuf2, (s32)usRez2);
//    OS_ENTER_CRITICAL();
//    OSSchedLock();
//    OSTaskChangePrio(11,8);
    Write1_Poll((s8*)"  Signal_2 = ");
    Write1_Poll((s8*)&cBuf2[5]);
    Write1_Poll((s8*)" mV\n");
//    OS_EXIT_CRITICAL();
//    OSSchedUnlock();
//    OSTaskChangePrio(8,11);
    ucSend2=0;
    ucSend=ucSend1+ucSend2;       //  Для наблюдения времени передачи
     __NOP();
  }   //  В Задаче должен быть бесконечный цикл, здесь его конец
}     //  Здесь заканчивается тело функции, на базе которой создается Задача



//----------------- Вспомогательные функции --------------------------------
//  Преобразование 32-битовой величины со знаком в 10-чную ASCII-строку
#define FALSE 0
#define TRUE !FALSE

s8* Dec_Convert(s8* buf, s32 value) 
{
	int divider = 1000000000;     
	unsigned char bNZflag=FALSE, minus=FALSE;		//  Флаги левых нулей и минуса
	unsigned char current_digit;

	if (value < 0) 
  {		//    Если число value отрицательное 
		minus=TRUE;
		value = -value;
	}
	while (divider) 
  {
		current_digit = value / divider;
		if (current_digit || bNZflag) 
    { //  Как только получили ненулевую цифру,
		  	if (minus) 
        { 	      //  Если число отрицательное, то поставим -
		    	buf--;
          *buf++ = '-';
          minus=FALSE; 
		  	} 
			value %= divider;
			*buf++ = current_digit + '0';
			bNZflag = TRUE;				// это значит, что левые нули закончились
		} 
    else 
    {  			//  Вместо левых нулей - пробелы, чтобы выровнять вправо
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
  while (*ptr) 
  {
    USART_SendData(USART1, *ptr);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) { }
    ptr++;  
		len++;
//    while (GetChar1() != ' ') { }    //  Ожидание приема пробела
  }
  return len;
}



//  !!!  Функции ???Hook() при необходимости можно раскомментировать и наполнить действиями
//       он должен здесь быть.

void  App_TaskCreateHook (OS_TCB *ptcb) { }
void  App_TaskDelHook (OS_TCB *ptcb) { }
void  App_TaskIdleHook (void) { }
void  App_TaskStatHook (void) { }
void  App_TaskSwHook (void) { }
void  App_TCBInitHook (OS_TCB *ptcb) { }
void  App_TimeTickHook (void) { }

