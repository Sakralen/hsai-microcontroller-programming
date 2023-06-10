/*=============================================================================*\
  APN: Тестирование работы с параллельнми портами
  Смотри ремарки в файле <Параллельные порты в STM32F10x.docx>
  
  Файл ParP_SysTick - вариант программы управления светофором, в котором
использовано определение функций отдельных битов и выводов порта для определенных
действий, так, что манипуляции с уже назначенными для определенной функциональности
битами, не приводят к влиянию на прочие биты, как не задействованные, так и на 
задействованные.
   Определения битов, используемых для управления светофором вынесено в заголовочный 
файл  ParP. h.
   Демонстрируется использование простого таймерного канала.

  
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
#include "ParP.h"

    // ---------- Global variables -----------------------------------------
u32 Diagr[][2] =                  //  Массив, описывающий диаграмму работы светофора
{   NS_Red|WO_Green,      GR_T,   // Количество фаз теперь можно задать любым.
    NS_Yellow|WO_Yellow,  YY_T,   //
    NS_Green|WO_Red,      GR_T,   //
#ifdef PEDESTRIANS
    NS_Yellow|WO_Red,     YY_T,
    NS_Pdst|WO_Pdst|NS_Red|WO_Red, GR_T,   //  Пешеходный такт
#endif
    NS_Yellow|WO_Yellow,  YY_T,   //
    0,                    0
};

u32 uiA, uiVar2;
u32 uiT1, uiT2, uiTp, uiDT;       //  Для измерения времен

    // ----- Function definitions --------------------------------------------
void Delay(u32 uiTDel) {while (uiTDel--) {} }


    //  Обработчик прерывания от SysTick
void SysTick_Handler(void) { 
  GPIOC->ODR ^= GPIO_Pin_8;           //  Перекл. PC.8
	for (uiVar2=10; uiVar2>0; uiVar2--) {};  //  Зачем эта задержка???
}



    // ===== Function main() =================================================
int main(void)
{  // Manage PortC	
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN;        
      //  Разрешение тактирования порта А (опрос кнопки USER) и порта С (лампы светофора)

  GPIOC->CRL = 0x33333333;     // Выводы 0...7 порта С - выходы 50 МГц для светофора
  GPIOC->CRH = 0x00000033;     // Выводы 8,9 порта С - для управления в обработчике
  
    //  Настройка таймера SysTick
  SysTick->LOAD = 0xFFFFFF;    //  Константа перезагрузки  SystemCoreClock  
  SysTick->CTRL = 5;           //  Пуск при входной частоте = Ядро/8

    //  Определение систематической ошибки
  uiT1 = SysTick->VAL;
  uiT2 = SysTick->VAL;
  uiTp = uiT1-uiT2;

    //  Определим коэффициент для программы задержки
  uiT1 = SysTick->VAL;
  Delay(1 ms);
  uiT2 = SysTick->VAL;
  SysTick->CTRL = 0;
  uiDT = uiT1-uiT2-uiTp;

	SysTick->LOAD = 12000;      
  SysTick->CTRL = 7;          //  При повторном пуске таймера загрузка LOAD
  Delay(5000000);
  SysTick->CTRL = 0;
   
  uiA = SvetMask;
  int i;
  while(1) {
    i=0;
    do {          //  Зажечь комбинацию ламп, соответственно такту
      GPIOC->BSRR = ((Diagr[i][0])&uiA) | ((~(Diagr[i][0])&uiA))<<16; //  Управление через BSRR
      if (GPIOC->ODR&NS_Pdst)
        {SysTick->CTRL = 7;}
      Delay(Diagr[i][1]);
      SysTick->CTRL = 0;
      i++;
    } while (Diagr[i][1]!=0);     
		while(((GPIOA->IDR)&1)==1);   //  Остановка в конце диагр. по нажатию кнопки USER
  }
//	return 0;
}
