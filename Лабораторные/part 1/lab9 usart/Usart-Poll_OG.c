/*=============================================================================*\
  
	Проверка скорости UART
	В0 соединён с А9 (передатчик UART)
	Обработчик прерывания (В0) считает интервалы передачи бита (10101010 0х55)
	Задержка в строке 100 нужна, чтоб не вызывалось лишенее прерывание при инициализации UART
  
		
\*=============================================================================*/

//#include "stm32f10x.h"
#include <stm32f10x.h>


#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_exti.h"

#define GPIO_SetRstBits(GPIOx,SetPin,RstPin) GPIOx->BSRR=SetPin|(RstPin<<16)

//u8 usMes1[]= {0x55,0};         //  Определены две текстовые строки для передачи
//u8 usMes2[]= {0x41,0};
u8 usMes1[]= {0xc0,0x20,0xf8,0xee,0x20,0xec,0xfb,0x20,0xed,0xe0,0xef,0xe8,0xea,0xe0,0xeb,0xe8,0x0a,0};
u8 usMes2[]= {0};

u8 symb, counter;

int iArr[20] = {0};
int iArr2[20] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
	int ggg = 0;

	int ibit_counter = 0;
int iedge_counter = 0;
double v1,v2, v3, v4, v5;

int32_t  siLen;           //  Для числа переданных символов, возвращаемого функцией Write1_Poll(uint16_t *)

void Delay(int i)
{
	while (i--){}
}

GPIO_InitTypeDef GPIO_InitStructure;    //  Для настройки порта PA.9 в реж. альт-фун для Tx USART1
USART_InitTypeDef USART_InitStructure;  //  Для настройки параметров USART1

u16 Write1_Poll(u8*);        //  Объявление функции передачи строки символов
u8 GetChar1(void);               //  Объявление функции приема одного символа      

int main(void)  { 

	  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;        // p.89

    //  Включение режима "вывод" для светодиода (4 - вход, 3 - выход) // p. 101, 110
  GPIOC->CRH = 0x44444443;  //  Вывод PC8 - для управления USER СвДиод
 
	
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_AFIOEN , ENABLE);

	
  //  Включение тактирования порта PA, подсистемы USART1 и схемотехники регистров альт.функций	 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);   

  /* Configure USARTy Tx as alternate function push-pull */
																										// p.157 Alternate functions (AF) RefMaual.pdf,
																										// p.176 USART1 remapping RefMaual.pdf,
																										// p.181 ((TX/PA9, RX/PA10)) RefMaual.pdf,
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9 | GPIO_Pin_10;       //  Выход Tx USART1 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;

  GPIO_Init(GPIOA, &GPIO_InitStructure);
	      //  Задание настроечных параметров для USART1
        //  Скорость можно задать не меньше 600
  USART_InitStructure.USART_BaudRate   = 2400;    //   p.792 Baud rate register RefMaual.pdf,
	USART_InitStructure.USART_WordLength = USART_WordLength_8b; // p.793 USART_CR1 Bit 12 M: Word length
  USART_InitStructure.USART_StopBits   = USART_StopBits_1;    // p.795 USART_CR2 Bits 13:12 STOP: STOP bits
  USART_InitStructure.USART_Parity     = USART_Parity_No;     // p.793 USART_CR1 Bit 10 PCE: Parity control enable
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // p.796 USART_CR3 Bit 9 CTSE: CTS enable
  USART_InitStructure.USART_Mode       = USART_Mode_Rx | USART_Mode_Tx; // p.794 USART_CR1 Bit 2,3 Transmitter, receiver enable
  
	USART_Init(USART1, &USART_InitStructure);   //
  USART_Cmd(USART1, ENABLE);  // p.793 USART_CR1 Bit 13 UE: USART enable

		Delay(10000);

	while(1) 
	{						//  Бесконечный цикл, обычный для управляющей программы
			siLen = Write1_Poll(usMes1);
      Delay(1000000);		
			siLen = Write1_Poll(usMes2);
      Delay(1000000);	
		
		 /*symb = GetChar1();
		
		 counter = 0;
		
		 if (symb == 0x55)
			 counter = 30;
		 else if (symb == 0x33)
			 counter = 6;

		for(int i = 0; i<counter; i++)
		{
  			  GPIOC->ODR  ^= 0x100;      // p.112
		   		Delay(1000000);				
		}

			Delay(50000000);
		*/
  }	
}

/*====================================================*\
    Write Func
|*====================================================*/

//  Функция передачи строки через USART1 с поллингом без проверок параметров
u16 Write1_Poll(u8* ptr) {
  int32_t len = 0;
  while (*ptr) {
		// p.792 Data register (USART_DR) Bits 8:0 DR[8:0]: Data value
    USART_SendData(USART1, *ptr);
		//GPIOC->ODR ^= 1 << 8;
		// p.790 USART_SR Bit 7 TXE: Transmit data register empty
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) { }
    ptr++;  
		len++;

  }
  return len;
}

u8 GetChar1(void) {
		// p.790 USART_SR Bit 5 RXNE: Read data register not empty
  while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET) { }
  return USART_ReceiveData(USART1);
}  
  
