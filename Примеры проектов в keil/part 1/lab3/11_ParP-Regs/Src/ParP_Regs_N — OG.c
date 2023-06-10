/*=============================================================================*\
  APN: Тестирование работы с параллельнми портами на платах Nucleo и Discovery
c МК STM32F10xRB
  
  Конфигурация [ParP-Regs] - использует технику работы с параллельными
портами путем обращения напрямую к регистрам.

на Nucleo    - СвДиод (зел), подключен к выводу PA5, 
             - Кнопка USER подключена к выводу PC13

\*=============================================================================*/

// ---- Директивы #include подключают заголовочные файлы -----------------------
#include <stm32f10x.h>

// ---- Директивы #define - определения для 

#define PA0ON 	GPIOA->ODR |= 0x1;
#define PA0OFF 	GPIOA->ODR &= 0xfffe;
#define PA0REV	GPIOA->ODR ^= 1; 

#define PA1ON 	GPIOA->ODR |= 0x2;
#define PA1OFF 	GPIOA->ODR &= 0xfffd;
#define PA1REV	GPIOA->ODR ^= 1<<1; 

#define PA2ON 	GPIOA->ODR |= 0x4;
#define PA2OFF 	GPIOA->ODR &= 0xfffb;
#define PA2REV	GPIOA->ODR ^= 1<<2; 

#define PA3ON 	GPIOA->ODR |= 0x0008;
#define PA3OFF 	GPIOA->ODR &= 0xfff7;
#define PA3REV	GPIOA->ODR ^= 1<<3;

#define PA4ON 	GPIOA->ODR |= 0x0010;
#define PA4OFF 	GPIOA->ODR &= 0xffef;
#define PA4REV	GPIOA->ODR ^= 1<<4;

//G
#define PA5ON 	GPIOA->ODR |= 0x0020;
#define PA5OFF 	GPIOA->ODR &= 0xffdf;
#define PA5REV	GPIOA->ODR ^= 1<<5;

//Y
#define PA6ON 	GPIOA->ODR |= 0x0040;
#define PA6OFF 	GPIOA->ODR &= 0xffbf;
#define PA6REV	GPIOA->ODR ^= 1<<6;

//R
#define PA7ON 	GPIOA->ODR |= 0x0080;
#define PA7OFF 	GPIOA->ODR &= 0xff7f;
#define PA7REV	GPIOA->ODR ^= 1<<7;

#define PA13ON GPIOA->ODR |= 0x2000;
#define PA14ON GPIOA->ODR |= 0x4000;
#define PA15ON GPIOA->ODR |= 0x5000;

#define leaveP567 0x00e0
#define leaveP012 0x0007

// ---- Глобальные переменные -----------------------------------------
GPIO_InitTypeDef port;

// ----- Function definitions --------------------------------------------
void Delay(u32 uiTDel) 
  {
    while (uiTDel--) {} 
  }	

// ===== Function main() =================================================
int main(void)
{  	//  Разрешение тактирования порта А и порта С 
    // (к (разным)выводам этих портов на платах подключены кнопка User и СвДиоды)
  //RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN;  

	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	

	//  Включение режима "вывод" для светодиода
	//GPIOA->CRL = 0x44344444;  //  Вывод PA5 - для управления USER СД Nucleo
	
	//#define GPIOA               ((GPIO_TypeDef *) GPIOA_BASE)
	//#define GPIOA_BASE            (APB2PERIPH_BASE + 0x0800)
	//#define APB2PERIPH_BASE       (PERIPH_BASE + 0x10000)
	//#define PERIPH_BASE           ((uint32_t)0x40000000)
	/*typedef struct
{
  __IO uint32_t CRL;
  __IO uint32_t CRH;
  __IO uint32_t IDR;
  __IO uint32_t ODR;
  __IO uint32_t BSRR;
  __IO uint32_t BRR;
  __IO uint32_t LCKR;
} GPIO_TypeDef;*/

	///////////////////////////////////////////////////
	
		RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;  //тактирование порта А (CMSIS)
		//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	//тактирование порта C (SPL)
	
		GPIOA->CRL = 0x44344443;	//PA5 & PA6 & PA7 (CMSIS)
		
		/*uint32_t* ptr = (uint32_t*)(0x40000000 + 0x10000 + 0x0800);
		*ptr = 0x34344444; //Сконфигурировали PA5,7 на pp (вручную)
		
		GPIO_StructInit(&port);
    port.GPIO_Mode = GPIO_Mode_Out_PP;
    port.GPIO_Pin = GPIO_Pin_6;
    port.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &port); //Сконфигурировали PA6 на pp (SPL)
		
		uint32_t* ptr1 = (uint32_t*)(0x40000000 + 0x10000 + 0x0800 + 0x0C);
		*ptr1 ^= 1<<5;
		*ptr1 ^= 1<<5;*/
	
		unsigned int mask = 0;
		

			
			RCC->APB2ENR |= RCC_APB2ENR_AFIOEN; 				//Тактируем для EXTI
			AFIO->EXTICR[0] &= ~(AFIO_EXTICR1_EXTI0); 	//PA0 как источник прерываний для EXTI
			EXTI->RTSR |= EXTI_RTSR_TR0; 								//Прерывания по восх. фронту
			NVIC_EnableIRQ(EXTI0_IRQn);   							//Разрешили прерывания
			EXTI->PR = EXTI_PR_PR0;      								//Сбросили флаг на всякий случай        
			EXTI->IMR |= EXTI_IMR_MR0;   								//Включаем
	
			

  while(1) {
		
		GPIOA->ODR ^= 1;
		Delay(10000000);
		
//		PA5OFF
//		PA6ON
//		Delay(10000000);
//		//PA6OFF
//		//PA7ON
//		GPIO_ResetBits(GPIOA, GPIO_Pin_6);
//		GPIO_SetBits(GPIOA, GPIO_Pin_7);
//		Delay(10000000);
//		//PA6ON
//		GPIO_WriteBit(GPIOA, GPIO_Pin_6, 1);
//		Delay(10000000);
//		//PA7OFF
//		//PA6OFF
//		GPIO_Write(GPIOA, 0);
//		PA5ON
//		Delay(10000000);

		
		//GPIO_SetBits(GPIOA, GPIO_Pin_5); //Включили PA5 (SPL)
		//Delay(100000);
		//GPIO_ResetBits(GPIOA, GPIO_Pin_5); //Выключили PA5 (SPL)
		
		////////////////// Хитрый способ
//		mask += 0x0040;
//		mask &= leaveP567;
//		
//		GPIOA->ODR = mask;
//		
//		if (mask == 0) PA5ON
//			
//		Delay(10000000);
//		
//		PA5OFF
		///////////////////
		
		//GPIOA->ODR ^=1<<5;      //  USER СвДиод Nucleo    (1 - горит)
		
		//Delay(1000000);
		
            //  Остановка в конце диагр. по нажатию кнопки USER
		//while(((GPIOC->IDR)&(1<<13))==(1<<13))
    //  {              //  USER кнопка Nucleo к PC13 (нажата - 0)
    //    __NOP();    // см. core_cmInstr.h, строки 321++
    //  }             // чтобы можно было поставить "точку останова"
  }
//	return 0;
}

void EXTI0_IRQHandler(void) {
	EXTI->PR = EXTI_PR_PR0;
	GPIOA->ODR ^= 1<<5;
	Delay(1000000);
	GPIOA->ODR ^= 1<<5;
}

//void EXTI5_IRQHandler(void) {
//	EXTI->PR = EXTI_PR_PR5;
//	GPIOA->ODR |= 1;
//	GPIOA->ODR ^= 1;
//}

//void EXTI6_IRQHandler(void) {
//	EXTI->PR = EXTI_PR_PR6;
//	GPIOA->ODR |= 1;
//	GPIOA->ODR ^= 1;
//}

//void EXTI7_IRQHandler(void) {
//	EXTI->PR = EXTI_PR_PR7;
//	GPIOA->ODR |= 1;
//	GPIOA->ODR ^= 1;
//}
