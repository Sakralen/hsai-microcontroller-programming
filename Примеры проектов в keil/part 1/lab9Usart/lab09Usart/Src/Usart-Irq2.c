/*=============================================================================*\
  
      Файл Usart-Irq2 проекта-демонстрации по использованию USART
  Двухсторонний обмен с использованием асинхронного режима USART1. 
  В обработчике прерывания от USART 
	
\*=============================================================================*/

//#include "stm32f10x.h"
#include <stm32f10x.h>

#define GPIO_SetRstBits(GPIOx,SetPin,RstPin) GPIOx->BSRR=SetPin|(RstPin<<16)
#define TXEIE (1<<7)
#define RXNEIE (1<<5)

uint8_t usMes1[]="Hello! \xA\xD Press 1 or 2\xA\xD";
uint8_t usMes2[]="  Received 1\xA\xD";
uint8_t usMes3[]="    Received 2\xA\xD";
int32_t  siLen, siI1, siA=500000;

uint8_t * pTx = 0;     //  Для чтения символов из строки
uint8_t RxByte = 0;    //  Для принимаемого байта


GPIO_InitTypeDef GPIO_InitStructure;    //  Для настройки порта A в реж. альт-фун для USART1
USART_InitTypeDef USART_InitStructure;  //  Для настройки параметров USART1
NVIC_InitTypeDef NVIC_InitStructure;    //  Для настройки контроллера прерываний

int32_t Write1_Poll(uint16_t *);
uint16_t GetChar1(void);

int main(void)  {  	
                          // 	Clocking
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);   

  /* Configure USARTy Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;         //  Выход Tx USART1
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;

  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
    //  Задаем параметры инициализации USART1
  USART_InitStructure.USART_BaudRate = 1200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  
    //  Конфигурируем USART1
  USART_Init(USART1, &USART_InitStructure);
  USART_Cmd(USART1, ENABLE);    //  После включения все прерывания от USART запрещены

    //  Настраиваем контроллер прерываний  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);     //  Priority Grouping
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;   //  Прер.от USART1
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;     //  Разрешаем
  NVIC_Init(&NVIC_InitStructure);                     // Собственно настройка

  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);    

  pTx = usMes1;   //  Инициализировать указатель и разрешить запрос от Tx
  USART_ITConfig(USART1, USART_IT_TXE, ENABLE);    
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);    

	while(1) {										//  Infinite Loop
    for(siI1=siA; siI1>3; siI1-=2) {
      siI1++;       //  Делаем здесь полезную работу
    }

    if (RxByte=='1') {   //  Запустить передачу
      RxByte=0;
      pTx = usMes2;   //  Инициализировать указатель и разрешить прерыв. от Tx
      USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
    }
      
    if (RxByte=='2') {   //  Запустить передачу
      RxByte=0;
      pTx = usMes3;   //  Инициализировать указатель и разрешить прерыв. от Tx
      USART1->CR1 |= (TXEIE);   //  Вот так можно сразу разрешать/запрещать несколько
    }
  }
	//	return 0;
}

//  Обработчик прерывания от USART1
void USART1_IRQHandler(void) { 
      //  Если установлен флаг передатчика  Tx
  if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {   //  Если Tx
    if(*pTx!=0)         //  Если не конец строки
      USART_SendData(USART1, *pTx++); //  передать следующий символ
    else 
    {   //  Строка передана, надо запретить запрос от Тх
//      USART_ITConfig(USART1, USART_IT_TXE, DISABLE);  //  функцией SPL
       USART1->CR1 &= ~TXEIE;   //  или прямым обращением к регистру
      pTx = 0;      //  Можно проверять на нуль-пойнтер
    }    
  }

  //*  Если установлен флаг приемника Rx
  if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
    RxByte = USART_ReceiveData(USART1);
  }
  // */ 
}

