#include "stm32f10x.h" 
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_conf.h"


//Переменная для хранения предыдущего состояния вывода PB0

GPIO_InitTypeDef port;
TIM_TimeBaseInitTypeDef timer;
EXTI_InitTypeDef exti;

/*******************************************************************/

void Delay(int i)
{
  while (i--);
}

int t2_start = 0;
int t2_fin = 0;

int t3_start = 0;
int t3_fin = 0;

int t4_start = 0;
int t4_fin = 0;


int intT3 = 0, intT4 = 0, intT2 = 0;

int start, end, res;

int main()
{
	  __enable_irq();
	//__disable_irq();

	    //Включаем тактирование порта GPIOA
	  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    //Включаем тактирование таймеров TIM3, TIM4   
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	
		////Включаем тактирование таймеров TIM2
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	 //настраиваем PA0 как выход
  	GPIO_StructInit(&port);
    port.GPIO_Mode = GPIO_Mode_Out_PP;
    port.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2; //еще и PA2
    port.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &port);
  
    //настройка таймера TIM4
    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Prescaler = SystemCoreClock/10000 - 1; 	     //предделитель
		timer.TIM_CounterMode = TIM_CounterMode_Up;
    timer.TIM_Period = 0;
    TIM_TimeBaseInit(TIM4, &timer);	     //Инициализация TIM4

	  //настройка таймера TIM3
    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Prescaler = SystemCoreClock/10000 - 1 ; 	     //предделитель
    timer.TIM_Period = 1000;
    TIM_TimeBaseInit(TIM3, &timer);	     //Инициализация TIM3
		
		//настройка таймера TIM2
		TIM_TimeBaseStructInit(&timer);
		timer.TIM_Prescaler = SystemCoreClock/10000 - 1;
		timer.TIM_Period = 1000;
		TIM_TimeBaseInit(TIM2, &timer);	     //Инициализация TIM2
	
    //Настраиваем таймеры для генерации прерывания по переполнению
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);	
		TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
		TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);	
		
		// Interrupt priority _STM32F103-ProgManual.pdf p.118, p.119, p.125
 //1
//    
		

 //2
  //  NVIC_SetPriority(TIM2_IRQn, 5);
	//	NVIC_SetPriority(TIM3_IRQn, 4);
	//	NVIC_SetPriority(TIM4_IRQn, 4);
		
	// Interrupt priority grouping _STM32F103-ProgManual.pdf p.37, p.135
	
	//3
//	NVIC_SetPriorityGrouping (5);  //SCB->AIRCR
//  NVIC_SetPriority(TIM2_IRQn, 4|3);  //NVIC->IP[28]
//	NVIC_SetPriority(TIM3_IRQn, 4|2);  //NVIC->IP[29]
//  NVIC_SetPriority(TIM4_IRQn, 4|2);  //NVIC->IP[30]
	
	NVIC_SetPriorityGrouping (7);  //SCB->AIRCR
  NVIC_SetPriority(TIM2_IRQn, 1);  //NVIC->IP[28]
	NVIC_SetPriority(TIM3_IRQn, 2);  //NVIC->IP[29]
  NVIC_SetPriority(TIM4_IRQn, 3);  //NVIC->IP[30]


//  NVIC_SetPriorityGrouping (5);

//		//NVIC->ISER[0] |= 1<<30; 
//		TIM4->PSC = 35999;
//		TIM4->ARR = 999;
//		TIM4->DIER |= TIM_IT_Update;
//		//TIM4->CR1 |= 1; 	
//		
//		//NVIC->ISER[0] |= 1<<30; 
//		TIM3->PSC = 35999;
//		TIM3->ARR = 999;
//		TIM3->DIER |= TIM_IT_Update;
//		//TIM4->CR1 |= 1;
//		
//		//NVIC->ISER[0] |= 1<<30; 
//		TIM2->PSC = 35999;
//		TIM2->ARR = 999;
//		TIM2->DIER |= TIM_IT_Update;
//		//TIM4->CR1 |= 1;
		

		////Сняли флаги
		TIM2->SR &= 0xFFFE;
		NVIC->ICPR[0] = 1<<28;
		TIM3->SR &= 0xFFFE;
		NVIC->ICPR[0] = 1<<29;
		TIM4->SR &= 0xFFFE;
		NVIC->ICPR[0] = 1<<30;

		//Разрешаем прерывание	TIM4, TIM3		
		NVIC_EnableIRQ(TIM2_IRQn);
		NVIC_EnableIRQ(TIM3_IRQn);
		NVIC_EnableIRQ(TIM4_IRQn);
		
    //Запускаем таймеры 
		TIM_Cmd(TIM4, ENABLE);
    //TIM_Cmd(TIM3, ENABLE);	
		//TIM_Cmd(TIM2, ENABLE);	
		
//		//Разрешаем прерывание	TIM4, TIM3		
//		NVIC_EnableIRQ(TIM3_IRQn);
//		NVIC_EnableIRQ(TIM2_IRQn);
//		NVIC_EnableIRQ(TIM4_IRQn);
			
    while(1)
    {
			__NOP();
    }	
		return 0;
}



void TIM4_IRQHandler()  // CMSIS startup_stm32f10x_md.s
{	
	intT4  = 100;
t4_start++;
		
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);	
 	Delay(1000000);
	GPIOA->ODR ^= GPIO_Pin_0;

	//TIM_ClearITPendingBit(TIM4, TIM_IT_Update);	
t4_fin++;
intT4  = 0;
	// Если сброс оставить в конце - то флаг не успевает сброситься и прерывание вызывается снова ??
 // TIM_ClearITPendingBit(TIM4, TIM_IT_Update);  
/*	
	if(TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)  
	{	
   	 TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
  } 
  */

	/*
	https://my.st.com/public/Faq/Lists/faqlst/DispForm.aspx?ID=143&level=1&objectid=141&type=product&Source=/public/Faq/Tags.aspx?tags=%20interrupt	
*/
}

void TIM3_IRQHandler()  // CMSIS startup_stm32f10x_md.s
{	
	intT3  = 100;
t3_start++;	
	//	Delay(1000000);
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
 	GPIOA->ODR ^= GPIO_Pin_1;
	Delay(1000000);
	//TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
t3_fin++;	
	intT3  = 0;
}

void TIM2_IRQHandler()  // CMSIS startup_stm32f10x_md.s
{	
	intT2  = 100;
t2_start++;	
	//	Delay(1000000);
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
 	GPIOA->ODR ^= GPIO_Pin_2;
	Delay(1000000);
	//TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
t2_fin++;	
	intT2  = 0;
}