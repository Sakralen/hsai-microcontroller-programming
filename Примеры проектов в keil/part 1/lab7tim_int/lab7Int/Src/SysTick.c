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

//#define GR3ON 	GPIOA->ODR |= 0x0008;
//#define GR3OFF 	GPIOA->ODR &= 0xfff7;
#define GR3REV	GPIOA->ODR ^= 1<<3;

#define GR4ON GPIOA->ODR |= 1<<4;
#define GR4OFF GPIOA->ODR &= 0xffef;
#define GR4REV GPIOA->ODR ^= 1<<4;

//#define GR7ON 	GPIOA->ODR |= 0x0080;
//#define GR7OFF 	GPIOA->ODR &= 0xffdf;
#define GR7REV	GPIOA->ODR ^= 1<<7;

#define GR04OFF GPIOA->ODR &= ~0x11;



    // ---------- Global variables -----------------------------------------

u32 uiA, uiVar2;
u32 uiT1, uiT2, uiTp, uiTp2, uiDT, uiDT2;       //  Для измерения времен

u32 osc;
unsigned int mask1 = 0, mask2 = 1 << 6, counter;

    // ----- Function definitions --------------------------------------------
void Delay(u32 uiTDel) {while (uiTDel--) {} }

// Обработчик прерывания SysTick
void SysTick_Handler(void) { 
	if (GPIOA->ODR & 1) {
		GR3REV
	}
	else {
		GR7REV
	}
	counter++;
}

void TIM4_IRQHandler()
{	
	TIM4->SR &= 0xFFFE;
	
	osc = 100;
	
	GR04OFF
	
	//GR0OFF
	mask1 += 0x2;
	mask1 &= LEAVE3;
	GPIOA->ODR = mask1;
	if (mask1 == 0) { 
		GR0ON 
		SysTick->CTRL |= 1;
	}
		
	//GR4OFF
	mask2 += 0x2 << 4;
	mask2 &= LEAVE3 << 4;
	GPIOA->ODR |= mask2;
	if (mask2 == 0) {
		GR4ON
		SysTick->CTRL |= 1;
	}
		
	osc = 0;
}

    // ===== Function main() =================================================
int main(void)
{ 
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	GPIOA->CRL = 0x33333333; 

	__disable_irq ();
	__enable_irq ();  
	
	NVIC->IP[TIM4_IRQn] = 3<<4;
	SCB->SHP[11] = 5<<4;

	NVIC->ISER[0] |= 1<<30; 
	RCC->APB1ENR |= RCC_APB1Periph_TIM4;
	TIM4->PSC = 35999;
	TIM4->ARR = 1999;
	TIM4->DIER |= TIM_IT_Update; 	
	
	SysTick->CTRL = 2;
	//SysTick->LOAD = 0x89543F;
	SysTick->LOAD = 1499999;
	
	//TIM4->CR1 |= TIM_CounterMode_Down;
	
	TIM4->CR1 |= TIM_CR1_ENABLE;
	
	while(1)
	{
		//if (!(SysTick->CTRL & 1) && ((GPIOA->ODR & 1) || (GPIOA->ODR & (1 << 4)))) {
				////TIM4->CR1 &= 0xfffffffe;
				//SysTick->CTRL |= 1;
		//}
		if (counter == 5) {
				SysTick->CTRL &= 0xfffffffe;
				//TIM4->CR1 |= TIM_CR1_ENABLE;
				//TIM4->EGR |= 1;
				counter = 0;
		}
	}	
	//return 0;
}
