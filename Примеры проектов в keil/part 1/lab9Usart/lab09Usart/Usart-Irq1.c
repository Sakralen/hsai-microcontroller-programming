/*=============================================================================*\
  
	  Это должен быть основной файл для первой конфигурации,
	он должен содержать функцию main.c
	  Конфигурацию и данный файл следует переименовать, чтобы имена отражали
	содержание создаваемого нового проекта.
	
\*=============================================================================*/

//#include "stm32f10x.h"
#include <stm32f10x.h>

#define GPIO_SetRstBits(GPIOx,SetPin,RstPin) GPIOx->BSRR=SetPin|(RstPin<<16)

uint16_t usMes1[]=L"   Hello\n";
uint16_t usMes2[]=L"Good Bye\n";
int32_t  siLen;

GPIO_InitTypeDef GPIO_InitStructure;    //  Для настройки порта A в реж. альт-фун для USART1
USART_InitTypeDef USART_InitStructure;  //  Для настройки парам. USART1

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
  
  USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  
  /* Configure USARTy */
  USART_Init(USART1, &USART_InitStructure);
  USART_Cmd(USART1, ENABLE);


	while(1) {										//  Infinite Loop
    siLen = Write1_Poll(usMes1);
    while (GetChar1() != L'1') { }
    siLen = Write1_Poll(usMes2);
    while (GetChar1() != L'2') { }
     	}
	
//	return 0;
}


/*====================================================*\
    Write + Read Func-s
|*====================================================*/

//  Функция передачи строки через USART1 с поллингом без проверок параметров
int32_t Write1_Poll(uint16_t * ptr) {
  int32_t len = 0;
  while (*ptr) {
    USART_SendData(USART1, *ptr);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) { }
    ptr++;  
		len++;
  }
  return len;
}

uint16_t GetChar1(void) {
  while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET) { }
  return USART_ReceiveData(USART1);
}  
  
  
  
