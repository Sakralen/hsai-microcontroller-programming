#include "stm32f10x.h" 
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_conf.h"



GPIO_InitTypeDef port;

/*******************************************************************/

void Delay(int i)
{
  while (i--);
}




int main()
{

	
	    //Включаем тактирование порта GPIOA
	  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	 //настраиваем PA0 как выход
  	GPIO_StructInit(&port);
    port.GPIO_Mode = GPIO_Mode_Out_PP;
    port.GPIO_Pin = GPIO_Pin_5;
    port.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &port);
  

		
    while(1)
    {
			GPIO_SetBits(GPIOA, GPIO_Pin_5);
			Delay(10000);
      GPIO_ResetBits(GPIOA, GPIO_Pin_5);
			Delay(10000);
			
			__NOP();
    }	
		return 0;
}


