/*=============================================================================*\
    
  Файл ParP_SysTick - шаблон программы, демонстрирующий технику использования
таймерного канала и аппаратного прерывания по переполнению таймера
   Демонстрируется использование таймерного канала Systick.

  
\*=============================================================================*/

  //  Макрос для одновременного сброса+установки (разных) битов в порте
  // прямой записью в регистр GPIO_SetRstBits
#define GPIO_SetRstBits(GPIOx,SetPin,RstPin) GPIOx->BSRR=SetPin|(RstPin<<16)

#define PEDESTRIANS
#define ns *3/625
#define mcs *3000/625
#define ms *3000000/625

    // ----- #include directives ------------------------------------------
#include <stm32f10x.h>

#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"

#define TIM_CR1_ENABLE   1
#define TIM_CR1_UDIS_DIS 1 << 1
#define TIM_CR1_ARPE_DIS 1 << 1

    // ---------- Global variables -----------------------------------------

u32 uiA, uiVar2;
u32 uiT1, uiT2, uiTp, uiTp2, uiDT, uiDT2;       //  Для измерения времен
u32 uCounter = 0;

GPIO_InitTypeDef port;
    // ----- Function definitions --------------------------------------------
void Delay(u32 uiTDel) {while (uiTDel--) {} }

void SysTick_Handler(void) { 
  //uCounter++;
	 
	GPIOA->ODR ^= 1<<5;
}


    // ===== Function main() =================================================
int main(void)
{ 
	/*
	__disable_irq ();
  __enable_irq ();
  //_stm32F103x(8,b)-DataSheet-MenDens.pdf
	// fig.2 стр 12
    
	
	//  Настройка таймера SysTick 4.5.1. 151 STM32F103-ProgManual.pdf
	  // When the processor is halted for debugging the counter does not decrement
	SysTick->LOAD = 0xFFFFFF;    //  Константа перезагрузки  SystemCoreClock  
  SysTick->CTRL =7;           //  Пуск при входной частоте = Ядро/8 или на частоте ядра

    //  Определение систематической ошибки
  uiT1 = SysTick->VAL;
  uiT2 = SysTick->VAL;
  uiTp = uiT1-uiT2;
    //  Теперь можно выполнять измерения времени
  
		
	uiT1 = SysTick->VAL;
  Delay(1000);
  uiT2 = SysTick->VAL;
 // SysTick->CTRL = 0;
  uiDT = uiT1-uiT2-uiTp;
	*/
	
	__disable_irq();
  __enable_irq();
	
	SysTick->CTRL = 2;
	SysTick->LOAD = 8999999;
	
	GPIO_StructInit(&port);
	port.GPIO_Mode = GPIO_Mode_Out_PP;
	port.GPIO_Pin = GPIO_Pin_5;
	port.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &port); 
	
	SysTick->CTRL |= 1;
		
while(1)
{}	
}
