/*=============================================================================*\
  
\*=============================================================================*/

// ---- Директивы #include подключают заголовочные файлы -----------------------
#include <stm32f10x.h>
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_exti.h"

// ---- Директивы #define - определения для 

#define LEAVE3 7
//#define USEUSART

// ---- Глобальные переменные -----------------------------------------
#ifdef USEUSART
USART_InitTypeDef USART_InitStructure;
#endif
GPIO_InitTypeDef GPIO_InitStructure;

u8 maskLED = 4;

u8 usMesG[]= {0x47, 0};       
u8 usMesY[]= {0x59, 0};
u8 usMesR[]= {0x52, 0};

// ----- Function definitions --------------------------------------------

#ifdef USEUSART
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
#endif

void Delay(int i)
{
	while (i--){}
}

void TIM4_IRQHandler()
{	
	TIM4->SR &= ~0x1;

	maskLED = maskLED << 1;
	maskLED &= LEAVE3;
	if (!maskLED) { 
		maskLED = 1; 
	}
	GPIOA->ODR &= ~(7 << 5);
	GPIOA->ODR |= maskLED << 5;
}

void EXTI9_5_IRQHandler(void) {
	if((EXTI->PR >> 5) & 1) {
		EXTI->PR = 1<<5;
//		Write1_Poll(usMesG);
	}
	else if((EXTI->PR >> 6) & 1) {
		EXTI->PR = 1<<6;
//		Write1_Poll(usMesY);
	}
	else {
		EXTI->PR = 1<<7;
//		Write1_Poll(usMesR);
	}
	GPIOA->ODR ^= 1;
	Delay(1000);
	GPIOA->ODR ^= 1;
}

// ===== Function main() =================================================

int main(void) { 
	__disable_irq ();
	__enable_irq (); 
	
	//тактирование:
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE); 
	
	//настройка контактов для USART:
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9 | GPIO_Pin_10;      //TX/PA9, RX/PA10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;

	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//настройка контактов для LED:
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_0; //LED PA5-7
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//приоритеты:
	NVIC_SetPriority(TIM4_IRQn, 4);
	NVIC_SetPriority(EXTI9_5_IRQn, 3);

	//настройка таймера
	NVIC->ISER[0] |= 1<<30; 
	RCC->APB1ENR |= RCC_APB1Periph_TIM4;
	TIM4->PSC = 35999;
	TIM4->ARR = 1999;
	TIM4->DIER |= TIM_IT_Update;
	
#ifdef USEUSART	
	USART_InitStructure.USART_BaudRate   = 2400;   
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits   = USART_StopBits_1;    
  USART_InitStructure.USART_Parity     = USART_Parity_No;   
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode       = USART_Mode_Rx | USART_Mode_Tx; 
  
	USART_Init(USART1, &USART_InitStructure);  
#endif

	//настройка EXTI
	AFIO->EXTICR[1] &= ~(AFIO_EXTICR2_EXTI5 | AFIO_EXTICR2_EXTI6 | AFIO_EXTICR2_EXTI7); //PA5-7 как источник прерываний для EXTI
	EXTI->RTSR |= EXTI_RTSR_TR5 | EXTI_RTSR_TR6 | EXTI_RTSR_TR7; //Прерывания по восх. фронту
	NVIC_EnableIRQ(EXTI9_5_IRQn); 	//Разрешили прерывания
	EXTI->PR = 7 << 5; //Сбросили флаг на всякий случай
	
	//включение EXTI:
	EXTI->IMR |= EXTI_IMR_MR5 | EXTI_IMR_MR6 | EXTI_IMR_MR7; 			//Включаем
	
#ifdef USEUSART		
	//включение USART:
	USART_Cmd(USART1, ENABLE);
	Delay(10000);
#endif

	//включение таймера:
	TIM4->CR1 |= 1;
	
  while(1) {
		
	}
}
