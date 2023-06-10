
#include "stm32f10x.h"

void Delay(int i)
{
  while(i--);
}

void Sound (int iL, int iH) {
  int iC=iH;
  while (iL>0) {
    iL--;
    iC--;
    if (iC == 0) {
      GPIOB->ODR  ^= 1<<2;
      iC=iH;
    }
  }
  for (iC=100000;iC>0;iC--); // В конце можно сделать паузу 
	Delay(10000);
}  

GPIO_InitTypeDef port;

#define LOW 200
#define DELTA 80

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

int main()
{
	int iDel = 1000000;
  
    /*RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
	
	  GPIOA->CRL &= 0xffff0000;
	  GPIOA->CRL |= 0x52;  // пин 0А - выход см. стр 101 (клавиатура а1-3)

    GPIOC->CRL &= 0xfffff000;	// вход
	  GPIOC->CRL |= 0x888; // клавиатура с0-2
	  GPIOC->ODR |= 7;
	
		GPIOA->ODR |= 0xE;*/
	
		RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;  //тактирование порта А (CMSIS)
		//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	
		GPIO_StructInit(&port);
    port.GPIO_Mode = GPIO_Mode_Out_PP;
    port.GPIO_Pin = GPIO_Pin_2;
    port.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &port); //Сконфигурировали PB2 на pp (SPL)
		
		GPIO_StructInit(&port);
    port.GPIO_Mode = GPIO_Mode_Out_PP;
    port.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    port.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &port); //Сконфигурировали PB2 на pp (SPL)
		
		GPIO_StructInit(&port);
    port.GPIO_Mode = GPIO_Mode_Out_OD;
    port.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
    port.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &port); //Сконфигурировали PC10 на od (SPL)
		
		GPIO_StructInit(&port);
    port.GPIO_Mode = GPIO_Mode_IPU;
    port.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    port.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &port); //Сконфигурировали PC0-3 на ipu (SPL)
	
		/*GPIOC->CRL &= 0xffff0fff;
		GPIOC->CRL |= 0x8000;
		
		GPIOC->ODR &= 0xfff7;
		GPIOC->ODR |= 1<<3;*/
	
	/*
	while(22)
	{
				Sound(200000, 200);
				Sound(200000, 400);
				Sound(200000, 600);
				Sound(200000, 800);
				Sound(200000, 1000);
	}
	*/
    while(22) {	
			//GPIOA->ODR = 0xC;
			for (int i = 0; i < 3; i++) {
				GPIOC->ODR |= 7 << 10;
				GPIOC->ODR &= ~(1 << (i + 10)); 
			
				if(!(GPIOC->IDR & 1))
					Sound(200000, (LOW + DELTA * i)); 
				if(!(GPIOC->IDR & 2))
					Sound(200000, (LOW + DELTA * 3 + DELTA * i));
				if(!(GPIO_ReadInputData(GPIOC) & 4))	
					Sound(200000, (LOW + DELTA * 6 + DELTA * i));
				//if(!(GPIOC->IDR & 8))	
				if(!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3))
					Sound(200000, (LOW + DELTA * 9 + DELTA * i));
				//Delay(10000);
			}

		while(((GPIOC->IDR)&(1<<13))!=(1<<13)) {
			
			PA6ON
			Delay(10000000);
			//PA6OFF
			//PA7ON
			GPIO_ResetBits(GPIOA, GPIO_Pin_6);
			GPIO_SetBits(GPIOA, GPIO_Pin_7);
			Delay(10000000);
			//PA6ON
			GPIO_WriteBit(GPIOA, GPIO_Pin_6, 1);
			Delay(10000000);
			//PA7OFF
			//PA6OFF
			GPIO_Write(GPIOA, 0);
			PA5ON
			Delay(10000000);
			PA5OFF
			Delay(10000000);
		}
		
//			GPIOA->ODR = 0xA;
// ..
		}

}
