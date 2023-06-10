/*=============================================================================*\
  
	Проверка скорости UART
	В0 соединён с А9 (передатчик UART)
	Обработчик прерывания (В0) считает интервалы передачи бита (10101010 0х55)
	Задержка в строке 100 нужна, чтоб не вызывалось лишенее прерывание при инициализации UART
  
		
\*=============================================================================*/

// ---- Директивы #include подключают заголовочные файлы -----------------------
#include <stm32f10x.h>

// ---- Директивы #define - определения для 

#define LEAVE3 7
#define PSEUDO

// ---- Глобальные переменные -----------------------------------------
USART_InitTypeDef USART_InitStructure;
GPIO_InitTypeDef GPIO_InitStructure;

u8 maskLED = 4;

u8 usMesG[]= {0x47, 0x72, 0x65, 0x65, 0x6e, 0};       
u8 usMesY[]= {'Y', 'e', 'l', 'l', 'o', 'w', 0};
u8 usMesR[]= {'R', 'e', 'd', 0};
u8 usMesUn[]= {'U', 'n', 'd', 'e', 'f', 'i', 'n','e', 'd', 0};

u8 data;
u8 inpBuff[2];
u8 curBuffPos;

u8* msg;
u32 osc;

// ----- Function definitions --------------------------------------------

void Delay(int i)
{
	while (i--){}
}

void TIM4_IRQHandler()
{	
	TIM4->SR &= ~0x1;
#ifndef PSEUDO	
	maskLED += 2;
	maskLED &= LEAVE3;
	GPIOB->ODR &= ~(7 << 5);
	GPIOB->ODR |= maskLED << 5;
	if (!maskLED) { 
		GPIOB->ODR |= 1 << 5;
	}
#else
	maskLED = maskLED << 1;
	maskLED &= LEAVE3;
	if (!maskLED) { 
		maskLED = 1; 
	}
	GPIOB->ODR &= ~(7 << 5);
	GPIOB->ODR |= maskLED << 5;
#endif
}

void EXTI9_5_IRQHandler(void) {
	if(EXTI->PR & (1 << 5)) {
		EXTI->PR = 1 << 5;
		msg = usMesG;
		USART1->CR1 |= 1 << 7;
	}
	if(EXTI->PR & (1 << 6)) {
		EXTI->PR = 1 << 6;
		msg = usMesY;
		//USART1->CR1 |= 3 << 6;
		USART1->CR1 |= 1 << 7;

	}	
	if(EXTI->PR & (1 << 7)) {
		EXTI->PR = 1<<7;
		msg = usMesR;
		USART1->CR1 |= 1 << 7;
	}
}

void USART1_IRQHandler(void) {
	
	if (USART1->SR & (1 << 7)){	
			osc = 100;
			//Transmit data register empty
			//USART1->SR &= ~(1 << 7);
			if (*msg) {
				USART1->DR = *msg;
				msg++;
			}
			else {
				USART1->SR &= ~(1 << 7);
				USART1->CR1 &= ~(1 << 7);
			}
			osc = 0;
	}
	if (USART1->SR & (1 << 5)) { 		//Read data register not empty
		//USART1->SR &= ~(1 << 5);
		data = USART1->DR;
			switch(data) {
				case 'N':	//Next
					TIM4->EGR |= 1;
					break;
				case 'D': //Disable
					TIM4->CR1 &= ~1;
					break;
				case 'E':	//Enable
					TIM4->CR1 |= 1;
					TIM4->EGR |= 1;
					break;
				case 'G': //Green
					GPIOB->ODR ^= 1 << 5;
					break;
				case 'Y': //Yellow
					GPIOB->ODR ^= 1 << 6;
					break;
				case 'R':	//Red
					GPIOB->ODR ^= 1 << 7;
					break;
				case 'C':	//Clear all LEDs
					GPIOB->ODR &= ~(7 << 5);
					break;
				case 'L':	//Light up all LEDs
					GPIOB->ODR |= 7 << 5;
					break;
				default :
					break;
			}
		}
}
// ===== Function main() =================================================

int main(void) { 
	__disable_irq ();
	__enable_irq (); 
	
	//тактирование:
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE); 
	
	//настройка контактов для USART:
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;      //TX/PA9, RX/PA10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;

	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;      //TX/PA9, RX/PA10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;

//	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//настройка контактов для LED:
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7; //LED PA5-7
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//приоритеты:
	NVIC_SetPriority(USART1_IRQn, 1); 
	NVIC_SetPriority(TIM4_IRQn, 3);
	NVIC_SetPriority(EXTI9_5_IRQn, 2);
	

	//настройка таймера
	NVIC->ISER[0] |= 1<<30; 
	RCC->APB1ENR |= RCC_APB1Periph_TIM4;
	TIM4->PSC = 35999;
	TIM4->ARR = 1999;
	TIM4->DIER |= TIM_IT_Update;
	
	NVIC_EnableIRQ(USART1_IRQn);
	
	USART_InitStructure.USART_BaudRate   = 2400;   
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits   = USART_StopBits_1;    
  USART_InitStructure.USART_Parity     = USART_Parity_No;   
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode       = USART_Mode_Rx | USART_Mode_Tx; 
  
	USART_Init(USART1, &USART_InitStructure);
	
//  USART1->CR1 |= 1<<6; //Прерывания по флагу TC -- transmission complete
		USART1->CR1 |= 1<<5; //Прерывания по флагу RXNE -- read data register is not empty
		
//	USART1->SR &= ~(1<<5);
//	NVIC->ICPR[1] = 1 << 5; 

	//настройка EXTI
	AFIO->EXTICR[1] |= AFIO_EXTICR2_EXTI5_PB | AFIO_EXTICR2_EXTI6_PB | AFIO_EXTICR2_EXTI7_PB;
	EXTI->RTSR |= EXTI_RTSR_TR5 | EXTI_RTSR_TR6 | EXTI_RTSR_TR7; //Прерывания по восх. фронту
	NVIC_EnableIRQ(EXTI9_5_IRQn); 	//Разрешили прерывания
	
	//EXTI->PR = 7 << 5; //Сбросили флаги на всякий случай
	
	//включение EXTI:
	EXTI->IMR |= EXTI_IMR_MR5 | EXTI_IMR_MR6 | EXTI_IMR_MR7; 			//Включаем
	
	//включение USART:
	USART_Cmd(USART1, ENABLE);
	
	//Delay(10000);

	//включение таймера:
	TIM4->CR1 |= 1;
	
  while(1) {
	}
}