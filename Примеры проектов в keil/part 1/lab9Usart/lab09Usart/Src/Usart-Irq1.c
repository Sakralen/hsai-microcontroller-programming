/*=============================================================================*\
  
      Файл Usart-Irq1.c проекта-демонстрации по использованию USART
  Обмен с использованием асинхронного режима USART1. 
  Программа настраивает тактирование порта PA, интерфейса USART1 и схемотехники
  альтернативных функций. Затем конфигурируется вывод PA9 как выход USART1.Tx
  Конфигурируется USART1: 9600, 8бит, стоп.бит 1, чет.бита нет, HW FlCtrl нет,
  включены Tx и Rx. Включается USART1. 
  
  Далее смотрите комментарии в тексте программы и "Пояснения к проекту для учащихся.txt"
	
\*=============================================================================*/

#include "stm32f10x.h"
//#include <stm32f10x.h>

#define GPIO_SetRstBits(GPIOx,SetPin,RstPin) GPIOx->BSRR=SetPin|(RstPin<<16)
#define TXEIE  (1<<7)
#define RXNEIE (1<<5)

u8 usMes1[]="Hello\xA\xD";
u8 usMes2[]="GoodBye\xA\xD";
int32_t  siLen, siI1, siA=10000;

u8 * pTx = 0;     //  Для чтения символов из строки
u8 RxByte = 0;    //  Для принимаемого байта


GPIO_InitTypeDef GPIO_InitStructure;    //  Для настройки порта A в реж. альт-фун для USART1
USART_InitTypeDef USART_InitStructure;  //  Для настройки параметров USART1
NVIC_InitTypeDef NVIC_InitStructure;    //  Для настройки контроллера прерываний

int32_t Write1_Poll(u8*);
u8 GetChar1(void);

int main(void)  {  	
                          // 	Clocking
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);   

  //  Конфигурирует выход Tx USARTy Tx 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;         //  Выход Tx USART1
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
  
    //  Задаем параметры инициализации USART1
  USART_InitStructure.USART_BaudRate = 2400;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  
    //  Конфигурируем USART1
  USART_Init(USART1, &USART_InitStructure);
  USART_Cmd(USART1, ENABLE);  //  Разрешается работа USART1    

    //  После включения все прерывания от USART запрещены
    //  Настраиваем группинг прерываний  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);     //  Кол.уровней прио.=8
    //  Настраиваем контроллер прерываний  
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;   //  Прер.от USART1
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=7;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;     //  Разрешаем прерывание от USART1
    NVIC_Init(&NVIC_InitStructure);                   // Собственно настройка

	while(1) {										//  Infinite Loop
    for(siI1=siA; siI1>3; siI1-=2) {
      siI1++;       //  Здесь можно делать полезную работу
      __asm("nop");
//     __NOP;
    }
    //  Запустить передачу
    while(pTx!=0)   //  Проверка свободен ли USART
      {__asm("nop");}
    pTx = usMes1;     //  Инициализировать указатель и разрешить прерыв. от Tx
    USART_ITConfig(USART1, USART_IT_TXE, ENABLE); //  можно так (1)
                    //  Эта функция "умеет" 
                    //  разрешать/запрещать только один запрос
                    // Если обращаться напрямую к регистру USARTx->CR1, то можно
                    //  разрешать/запрещать сразу несколько запросов.
//    USART1->CR1 |= (TXEIE | RXNEIE);   // и можно прямым обращением к регистру  
                 //  так допустимо сразу разрешать/запрещать несколько запросов

    //  Делать другую полезную работу
    for(siI1=siA; siI1>3; siI1-=2) {
      siI1++;//  Делаем здесь полезную работу
    }
    while(pTx!=0)            //  Проверка  свободен ли USART
      {__asm("nop");}
    pTx = usMes2;               //  Задать адрес начала строки
//    USART_Cmd(USART1, ENABLE);  //  Включить USART и 
    USART_ITConfig(USART1, USART_IT_TXE, ENABLE); //  разрешить запрос от Тх
  }
	//	return 0;
}

  //  Обработчик прерывания от USART1
void USART1_IRQHandler(void) {  
  if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {   //  Если Tx
    if(*pTx!=0)         //  Если не конец строки
      USART_SendData(USART1, *pTx++);  //  передать следующий символ
    else 
    {     //  Если конец строки, надо
          // запретить запрос от Tx стандартной функцией SPL
//      USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
      USART1->CR1 &= ~TXEIE;   //... или прямой записью в регистр
      pTx = 0;      //  Можно будет проверять на нуль-пойнтер
    }    
  }
}


