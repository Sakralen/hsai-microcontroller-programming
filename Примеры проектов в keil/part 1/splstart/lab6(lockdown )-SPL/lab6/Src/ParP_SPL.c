
/*=============================================================================*\
  APN: Тестирование работы с параллельнми портами
  Смотри ремарки в файле <Параллельные порты в STM32F10x.docx>.
  Для отладки задать в опциях C/C++ USE_FULL_ASSERT USE_STDPERIPH_DRIVER
  Если не задано USE_FULL_ASSERT, то не делаются проверки параметров
  в функциях на этапе исполнения.
\*=============================================================================*/

#define GPIO_SetRstBits(GPIOx,SetPin,RstPin) GPIOx->BSRR=SetPin|(RstPin<<16)

#define PEDESTRIANS 

#include <stm32f10x.h>
#include "ParP.h"

    // ---------- Global variables -----------------------------------------
u32 Diagr[][2] =                  //  Массив, описывающий диаграмму работы светофора
{   NS_Red|WO_Green,      GR_T,   // Количество фаз теперь можно задать любым.
    NS_Yellow|WO_Yellow,  YY_T,   //
    NS_Green|WO_Red,      GR_T,   //
#ifdef PEDESTRIANS
    NS_Yellow|WO_Red,     YY_T,
    NS_Pdst|WO_Pdst|NS_Red|WO_Red,      GR_T,   //  Пешеходный такт
    NS_Red|WO_Yellow,  YY_T,   //
    0,                    0
#else
    NS_Yellow|WO_Yellow,  YY_T,   //
    0,                    0
#endif
};

u32 uiA;

GPIO_InitTypeDef gpioC;         //  Def Structure for Port SetUp globally
uint32_t uiTmp, uiA;

    // ----- Function definitions --------------------------------------------
void Delay(u32 uiTDel) {while (uiTDel--) {} }	

    // ===== Function main() =================================================
int main(void)
{  // Manage PortC	
  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);   
    // Clocking PortC On (21 without, 46 with checking)

    // Now fill in SetUp structure Fields
  	gpioC.GPIO_Mode = GPIO_Mode_Out_PP; 	//  as PushPull Output
  	gpioC.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
                     GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
  	gpioC.GPIO_Speed = GPIO_Speed_50MHz;

  	GPIO_Init(GPIOC, &gpioC);           //  Call Func to Setup PortC
  
    GPIO_Write(GPIOC, GPIO_Pin_8 | GPIO_Pin_9);

	uiTmp = GPIO_ReadInputData(GPIOC);
	
  uiA = SvetMask;
  int i;
  while(1) {
    i=0;
    do {          //  Зажечь комбинацию ламп, соответственно такту
//    GPIOC->BSRR = (~Diagr[i][0]) & uiA | ((Diagr[i][0])&uiA)<<16; //  Управление через BSRR
//    GPIO_ResetBits(GPIOC, GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7); 
      GPIO_ResetBits(GPIOC, SvetMask);
      GPIO_SetBits(GPIOC, ~Diagr[i][0]);  
      Delay(Diagr[i][1]);
      i++;
    } while (Diagr[i][1]!=0);     
//		while(((GPIOA->IDR)&1)==1);   //  Остановка в конце диагр. по нажатию кнопки USER
		while((GPIO_ReadInputData(GPIOA)&1)==1);   //  Остановка в конце диагр. по нажатию кнопки USER
  }
//	return 0;
}
