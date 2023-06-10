
/******************************************************************************************************\
*   Проект-прототип (пример) программы, написанной под ОС РВ uC/OS-II v.2.86
* Пример написан на базе демонстрационной программы разработчиков uC/OS  для архитектуры Cortex-M3
*
*   Filename      : uCOS_ADC_UART.c
* На симуляторе моделируется регистрация аналогового сигнала, который описан в файле Src\Input_Sig.ini
* 
*   Вначале все действия: 1) измерения, 2) накопление суммы, 3) усреднение, 4) преобразование среднего
* в строку, 5) передача через UART -- делаются в одной Задаче.
* Период дискретизации делается с использованием функции OSTimeDly(TDiscr1).
* При этом а) измерения не выполняются, пока идет передача, б) TDiscr задается неверно.
*   Положение можно исправить, если все "длинные" действия (4, 5) перенести в другую Задачу 
* с более низким приоритетом. 
* Теперь передача строки и измерения делаются параллельно, но если время передачи превышает
* период накопления серии, то передача "не успевает"      
*
\******************************************************************************************************/

#include "includes.h"
#include <stm32f10x.h>
   
#define SEND_TASK   //  Раскомментировать для создания отдельной Задачи-Передатчика

  //  На базе этих массивов будут (при работе системной функции OSTaskCreate(...)) созданы стеки Задач 
OS_STK         App_TaskStartStk[APP_TASK_START_STK_SIZE];

OS_STK         App_TaskMeas1Stk[APP_TASK_STK_SIZE];      //  Имя для размера стека, если надо менять для нескольких
OS_STK         App_TaskSend1Stk[APP_TASK_STK_SIZE];

  //  Функция для контроля размера стеков при отладке. Ей надо давать в качестве параметра
  // имя массива, на базе которой организован стек Задачи, например, вот так:
  // Var=FreeStkSpace(App_TaskStartStk);  
  //   В переменную Var вернется размер свободного места в Задаче TaskStart
u16 FreeStkSpace(OS_STK * x) 
{ OS_STK * pS  = x;
  OS_CPU_SR cpu_sr = 0; 
  u16 StkCtr   = 0;
    OS_ENTER_CRITICAL();              //  Для чего
    while (*pS==0) { StkCtr++; pS++;}
    OS_EXIT_CRITICAL();
    return StkCtr;
}

//  Объявления функций, на базе которых в программе будут созданы Задачи
//  Определения этих функций смотрите далее в этом файле 

static  void  App_TaskStart    (void  *p_arg);  //  Стартовая Задача
static  void  App_TaskMeas1    (void  *p_arg);  //  Прикладная Задача - измеритель

#ifdef SEND_TASK
 static  void  App_TaskSend1    (void  *p_arg); //  Задача-передатчик вначале не создается
#endif

unsigned char cTicks;    //  Служебная для наблюдения "тиков"
u8 ucSend=0, ucSend1=0, ucSend2=0;  //  Длит.передачи в UART

u16   usRez1  =0;     //  Переменная для результата1 
u8    cBuf1[20];      //  Буфер для строки результата1

s8* Dec_Convert(s8* buf, s32 value);
u16 Write1_Poll(s8* ptr);
CPU_INT08U  os_err; //  Переменная для кода завершения, используется в функциях ОС


/*******************************************************************************************************\
*                                                main()
\*******************************************************************************************************/
int  main (void)
{
//  BSP_IntDisAll();        //  Запрет всех прерываний, пока не будет настроена ОСРВ

  OSInit();               //  Инициализация uC/OS-II, The Real-Time Kernel
  OSTaskCreate((void (*)(void *)) App_TaskStart,    //  Создание стартовой Задачи 
               (void          * ) 0,
               (OS_STK        * )&App_TaskStartStk[APP_TASK_START_STK_SIZE - 1],
               (INT8U           ) APP_TASK_START_PRIO
              );
  OSStart();    //  Старт многозадачного режима (передача управления диспетчеру Задач)
                //  Возврата из функции OSStart() не произойдет никогда.
  return (0);   //  Сюда управление не попадет никогда
}


/*******************************************************************************************************\
*        App_TaskStart()
*
* Description : Стартовая (начальная) Задача. Она должна получить управление первой. Для этого ей
*   следует присваивать наивысший приоритет (в случае, если в main() вы создадите несколько Задач)
*   В стартовой Задаче (не раньше!!!) следует запустить системный таймер и разрешить от него прерывание
* 
*   Стартовая Задача - самое подходящее место для настроек периферийных подсистем 
   и для создания Задач прикладной программы
\*******************************************************************************************************/

static  void  App_TaskStart (void *p_arg)
{ //    CPU_INT32U  dly=12;
  u16 volatile usStkFree=APP_TASK_START_STK_SIZE;
  usStkFree=FreeStkSpace(App_TaskStartStk);   //  Контроль размера стека стартовой Задачи

  BSP_Init();   //  Фактически - только настройка RCC - 72 (или 24) МГц от ФАПЧ

  OS_CPU_SysTickInit();  // Запуск системного таймера и прерывания от его переполнения
        //  После этого SysTick работает, и многозадачность функционирует полностью
//------------------------------------------------------------------------------------

  //  Разрешить тактирование нужных подсистем
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA |  //  Порт PA - два входа аналоговых
                          RCC_APB2Periph_ADC1 |   //  АЦП
   //                       RCC_APB2Periph_AFIO |
                          RCC_APB2Periph_USART1,  //  UART
                            ENABLE);   

  //  Настроить выводы порта PA1 PA2 как аналоговые входы
    GPIO_InitTypeDef gpioA;
  gpioA.GPIO_Pin    = GPIO_Pin_1 | GPIO_Pin_2;    //  для подключения датчиков
  gpioA.GPIO_Mode   = GPIO_Mode_AIN;   
    GPIO_Init(GPIOA, &gpioA);
  
  //  Выход PA9 - Tx передатчик UART
  gpioA.GPIO_Pin = GPIO_Pin_9;         //  Выход Tx USART1
  gpioA.GPIO_Mode = GPIO_Mode_AF_PP;
  gpioA.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpioA);

    //  Настройка АЦП общая
    ADC_InitTypeDef ADC_InitStructure;
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;    //  Если два или более АЦП
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;         //  Режима сканирования нет
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;   //  Непрер.режима нет
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; //  Внеш.запуск ОТКЛ
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;  //  Рез. выровнен вправо
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
  USART_InitStructure.USART_BaudRate = 38400;  //38400;   //  Скорость 38 кбит/с 
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx;         //  Нужен передатчик
    USART_Init(USART1, &USART_InitStructure);   

  USART_Cmd(USART1, ENABLE);    //  Включили UART

 

//  Здесь самое подходящее место для создания пользовательских Задач, 
// объектов синхронизации-коммуникации и т.п.

//  Помните, что если у создаваемой Задачи приоритет выше, чем у стартовой Задачи, то
// вновь созданная Задача будет немедленно запущена на исполнение

  OSTaskCreate((void (*)(void *)) App_TaskMeas1,        //  Создадим Задачу для измерения1
               (void          * ) 0,
               (OS_STK        * ) &App_TaskMeas1Stk[APP_TASK_STK_SIZE - 1],  //  Стек Задачи
               (INT8U           ) APP_TASK_MEAS1_PRIO   // Уровень приоритета для этой Задачи =5
              );

#ifdef SEND_TASK
  os_err=OSTaskCreate( (void (*)(void *)) App_TaskSend1,        //  Создадим Задачу коммуникации
                        (void          * ) 0,
                        (OS_STK        * ) &App_TaskSend1Stk[APP_TASK_STK_SIZE - 1],
                        (INT8U           ) APP_TASK_SEND1_PRIO   // Уровень приоритета для этой Задачи =9
                   );     
    if(os_err!=OS_ERR_NONE) //  Если OS_MAX_TASKS == 3; - ошибки нет
    {
      __asm("nop");
      __NOP();
    }       //  Значения кодов ошибок в файле ucos_ii.h, со строки 245           
#endif

//  Уровни приоритета (целые в пределах от 0 до 254) должны быть различными для разных Задач
//  Константы уровней приоритета APP_TASK_MEAS1_PRIO, APP_TASK_SEND1_PRIO будут использоваться 
// как параметры в системных сервисах, управляющих Задачами для указания конкретной Задачи
  
  usStkFree=FreeStkSpace(App_TaskStartStk);   //  Проверка состояния стека App_TaskStart
  //  надо посмотреть значение переменной usStkFree, если оно близко к нулю, думать, что делать
  
    //  Задача App_TaskStart больше не нужна, с ней можно поступить несколькими способами, вот они:
//  OSTaskSuspend(OS_PRIO_SELF);      //  Системный сервис переводит Задачу App_TaskStart в ожидание 
    //  (2)   Можно удалить Задачу, если она больше не нужна, вот так
  OSTaskDel(APP_TASK_START_PRIO);   //  Здесь уровень приоритета (идентификатор удаляемой Задачи) указан явно
    //  (3)   Бесконечный цикл

//  OSTaskChangePrio(APP_TASK_START_PRIO, ??? NewPrio);   //  Можно изменить приоритет
  while (DEF_TRUE)   
  {     //  В Задаче должен быть бесконечный цикл
        //  В этот цикл можно поместить полезный код
        //  Надо лишь помнить, что приоритет стартовой Задачи был выше приоритета всех прочих созданных Задач
  }
    //  Но во всяком случае Задаче нельзя позволить завершиться выходом на закрывающую скобку:
}   // сюда попасть - недопустимо!!!


/*******************************************************************************************************\
*                                            App_TaskMeas1()
* Description : Задача выполняет измерения по одному каналу
*     с периодом дискретизации TDiscr1
* Уникальный уровень приоритета для этой Задачи  p_MEAS1 = 5 
\*******************************************************************************************************/

//  Параметры регистрации для сигнала1      (10 - 1 мс)
u16   TDiscr1   =10;  //  Период дискретизации сигнала1    (10)
u16   NSer1     =5;   //  Длина серии для сигнала1         (7)     
           // при (5) неисполнимо даже если передача в отдельной Задаче
u32   uiSum1    =0;   //  Для накопления суммы
u16   usCtr1    =0;   //  счетчик отсчетов в серии1
u16   usCtrSer  =0;   //  Счетчик серий
u8    cADC1     =0;   //  Для отладки (график в ЛА)

static void App_TaskMeas1(void *p_arg) 
{
  u16 volatile usStkFree=128;
  usStkFree=FreeStkSpace(App_TaskMeas1Stk);   //  Можно проверить стек

  while (DEF_TRUE)    //  Бесконечный цикл в Задаче 
  {    //  В Задаче должен быть бесконечный цикл
    cADC1=150;    //  Отладочная переменная для наблюдения времени измерения
      //  Включить канал АЦП
    //  Выбор канала 1, задание времени выборки
    //  Времена выборки: 1, 7, 13, 28, 41, 55, 71, 239
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_13Cycles5);

      //  Запустить АЦП функцией библиотеки SPL
    ADC_Cmd(ADC1, ENABLE);  
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)==RESET)   //  Ожидать результата 
    {
      __NOP();
    }
//    uiSum1+=ADC1->DR;   //  Прочитать результат измерения из регистра данных АЦП
    uiSum1+=ADC_GetConversionValue(ADC1);  // и прибавить к частичной сумме
    cADC1=0;      //  Конец измерения
    usCtr1++;     //  Счетчик измерений в серии
    if((usCtr1)==NSer1)   //  Если серия набрана
    { usCtr1=0;           //  Сброс счетчика измерений в серии
      usRez1=uiSum1*3300/4096/NSer1;      //  Не будет ли переполнения ????  
              //  Результат: Усреднение по серии и преобр. в миллиВольты
      usCtrSer++;         //  Счет числа серий (необязательно)
      uiSum1=0;           //  Обнуление частичной суммы   

//  Выполнить передачу. Если оставить передачу здесь, то пока она идет, измерения
// не будут выполняться. Вот основание для создания второй Задачи

#ifndef SEND_TASK
			{
				ucSend1=100;
				//  Преобразовать число в строку
				Dec_Convert((s8*)cBuf1, (s32)usRez1);
				Write1_Poll((s8*)"Signal_1 = ");
				Write1_Poll((s8*)&cBuf1[5]);
				Write1_Poll((s8*)" mV\n");
				ucSend1=0;
			}
#else
      OSTaskResume(APP_TASK_SEND1_PRIO);
#endif
    }
    OSTimeDly(TDiscr1);   //  Запустить тайм-аут на период дискретизации
          //  При каких условиях период измерений будет задан верно ???
    usStkFree=FreeStkSpace(App_TaskMeas1Stk);   //  Можно проверить стек
    __NOP();
  }
}

/*******************************************************************************************************\
*                                            App_TaskSend1()
* Description : Задача выполняет передачу отсчетов первого канала
* Уникальный уровень приоритета для этой Задачи   = 9 
\*******************************************************************************************************/

//*  Чтобы раскомментировать функцию передачи - поставьте "слэш" в начале этой строки
 
#ifdef SEND_TASK
static void App_TaskSend1(void *p_arg) 
{
  u16 volatile usStkFree=128;    //  Для проверки стека
  usStkFree=FreeStkSpace(App_TaskSend1Stk); //  Можно проверить стек

  while (DEF_TRUE) 
  {        //  В Задаче должен быть бесконечный цикл
    OSTaskSuspend(OS_PRIO_SELF);    //  Можно было указать приоритет APP_TASK_SEND1_PRIO
      //  Задача App_TaskSend1() будет продолжена, когда будут готовы данные для передачи
    __NOP();
    ucSend1=100;
      //  Преобразовать число в строку
    Dec_Convert((s8*)cBuf1, (s32)usRez1);
    Write1_Poll((s8*)"Signal_1 = ");
    Write1_Poll((s8*)&cBuf1[5]);
    Write1_Poll((s8*)" mV\n");
    ucSend1=0;
    __NOP();
  }   //  В Задаче должен быть бесконечный цикл, здесь его конец
}     //  Здесь заканчивается тело функции, на базе которой создается Задача
#endif

  // Не удаляйте и не изменяйте эту строку */


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


#if (OS_APP_HOOKS_EN > 0)
//  !!!  Функции ???Hook() при необходимости можно раскомментировать и наполнить действиями
//       они должны (могут) здесь быть.
/*
void  App_TaskCreateHook (OS_TCB *ptcb) { }
void  App_TaskDelHook (OS_TCB *ptcb) { }
void  App_TaskIdleHook (void) { }
void  App_TaskStatHook (void) { }
void  App_TaskSwHook (void) { }
void  App_TCBInitHook (OS_TCB *ptcb) { }
void  App_TimeTickHook (void) { }
*/

/*
*********************************************************************************************************
*                                      TASK CREATION HOOK (APPLICATION)
*
* Description : This function is called when a task is created.
*
* Argument(s) : ptcb   is a pointer to the task control block of the task being created.
*
* Note(s)     : (1) Interrupts are disabled during this call.
*********************************************************************************************************
*/

void  App_TaskCreateHook (OS_TCB *ptcb)
{
//#if ((APP_OS_PROBE_EN   == DEF_ENABLED) && \
//     (OS_PROBE_HOOKS_EN == DEF_ENABLED))
//    OSProbe_TaskCreateHook(ptcb);
//#endif
}

/*
*********************************************************************************************************
*                                    TASK DELETION HOOK (APPLICATION)
*
* Description : This function is called when a task is deleted.
*
* Argument(s) : ptcb   is a pointer to the task control block of the task being deleted.
*
* Note(s)     : (1) Interrupts are disabled during this call.
*********************************************************************************************************
*/

void  App_TaskDelHook (OS_TCB *ptcb)
{
    (void)ptcb;
}

/*
*********************************************************************************************************
*                                      IDLE TASK HOOK (APPLICATION)
*
* Description : This function is called by OSTaskIdleHook(), which is called by the idle task.  This hook
*               has been added to allow you to do such things as STOP the CPU to conserve power.
*
* Argument(s) : none.
*
* Note(s)     : (1) Interrupts are enabled during this call.
*********************************************************************************************************
*/

#if OS_VERSION >= 251
void  App_TaskIdleHook (void)
{
}
#endif

/*
*********************************************************************************************************
*                                        STATISTIC TASK HOOK (APPLICATION)
*
* Description : This function is called by OSTaskStatHook(), which is called every second by uC/OS-II's
*               statistics task.  This allows your application to add functionality to the statistics task.
*
* Argument(s) : none.
*********************************************************************************************************
*/

void  App_TaskStatHook (void)
{
}

/*
*********************************************************************************************************
*                                        TASK SWITCH HOOK (APPLICATION)
*
* Description : This function is called when a task switch is performed.  This allows you to perform other
*               operations during a context switch.
*
* Argument(s) : none.
*
* Note(s)     : (1) Interrupts are disabled during this call.
*
*               (2) It is assumed that the global pointer 'OSTCBHighRdy' points to the TCB of the task that
*                   will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCur' points to the
*                  task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/

#if OS_TASK_SW_HOOK_EN > 0
void  App_TaskSwHook (void)
{
//#if ((APP_OS_PROBE_EN   == DEF_ENABLED) && \
//     (OS_PROBE_HOOKS_EN == DEF_ENABLED))
//    OSProbe_TaskSwHook();
//#endif
}
#endif

/*
*********************************************************************************************************
*                                     OS_TCBInit() HOOK (APPLICATION)
*
* Description : This function is called by OSTCBInitHook(), which is called by OS_TCBInit() after setting
*               up most of the TCB.
*
* Argument(s) : ptcb    is a pointer to the TCB of the task being created.
*
* Note(s)     : (1) Interrupts may or may not be ENABLED during this call.
*********************************************************************************************************
*/

#if OS_VERSION >= 204
void  App_TCBInitHook (OS_TCB *ptcb)
{
    (void)ptcb;
}
#endif

/*
*********************************************************************************************************
*                                        TICK HOOK (APPLICATION)
*
* Description : This function is called every tick.
*
* Argument(s) : none.
*
* Note(s)     : (1) Interrupts may or may not be ENABLED during this call.
*********************************************************************************************************
*/

#if OS_TIME_TICK_HOOK_EN > 0
void  App_TimeTickHook (void)
{
//#if ((APP_OS_PROBE_EN   == DEF_ENABLED) && \
//     (OS_PROBE_HOOKS_EN == DEF_ENABLED))
//    OSProbe_TickHook();
//#endif
}
#endif




#endif
