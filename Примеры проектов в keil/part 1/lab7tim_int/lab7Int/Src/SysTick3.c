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

#define LEAVE3 7
#define GR0ON GPIOA->ODR |= 0x1;
#define GR0OFF GPIOA->ODR &= 0xfffe;
#define GR3ON 	GPIOA->ODR |= 0x0008;
#define GR3OFF 	GPIOA->ODR &= 0xfff7;
#define GR3REV	GPIOA->ODR ^= 1<<3;


    // ---------- Global variables -----------------------------------------

u32 uiA, uiVar2;
u32 uiT1, uiT2, uiTp, uiTp2, uiDT, uiDT2;       //  Для измерения времен

u32 osc;
unsigned int mask1 = 0, mask2 = 0x110 << 4, counter;

    // ----- Function definitions --------------------------------------------
void Delay(u32 uiTDel) {while (uiTDel--) {} }

// Обработчик прерывания SysTick
void SysTick_Handler(void) { 
	counter++;
	GPIOA->ODR ^= 1<<3;
	//SysTick->CTRL ^= 1;
	if (counter == 6) {
			SysTick->CTRL = 0;
			GR3OFF
			counter = 1;
			TIM4->ARR = 2001;
			TIM4->DIER ^= 1; //Разрешили прерывания
			TIM4->CR1 |= TIM_CR1_ENABLE;
	}
}

void TIM4_IRQHandler()
{	
	TIM4->SR &= 0xFFFE;
	
	GR0OFF
	mask1 += 0x2;
	mask1 &= LEAVE3;
	GPIOA->ODR = mask1;
	if (mask1 == 0) GR0ON
}

    // ===== Function main() =================================================
int main(void)
{ 
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	GPIOA->CRL = 0x33333333; 

	__disable_irq ();
	__enable_irq ();  

	          

	NVIC->ISER[0] |= 1<<30; 
	RCC->APB1ENR |= RCC_APB1Periph_TIM4;
	TIM4->PSC = 9999;
	TIM4->ARR = 2001;
	TIM4->DIER |= TIM_IT_Update; 	
	TIM4->CR1 |= TIM_CR1_ENABLE;
	
	counter = 1;
	
	while(1)
	{
		/*if (counter == 6) {
			SysTick->CTRL = 0;
			GR3OFF
			counter = 1;
			TIM4->ARR = 2001;
			TIM4->DIER ^= 1; //Разрешили прерывания
			TIM4->CR1 |= TIM_CR1_ENABLE;
		}*/
		if ((GPIOA->ODR & 1)) {
			TIM4->CR1 |= 1<<1;
			//TIM4->DIER ^= 1;
			/*TIM4->CR1 ^= 1; //Выключили
			TIM4->DIER ^= 1; //Запретили прерывания
			TIM4->SR &= 0xFFFE;	//Сняли запрос
			
			SysTick->LOAD = 0xfffff; //Загрузили время
			SysTick->CTRL = 3;			//Включили
			//while(counter != 6) {}*/
		}
	}	
	//return 0;
}
