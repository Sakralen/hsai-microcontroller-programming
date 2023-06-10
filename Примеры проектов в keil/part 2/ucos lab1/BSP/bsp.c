	/*
*********************************************************************************************************
*                                     MICIRUM BOARD SUPPORT PACKAGE
*
*                             (c) Copyright 2007; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
  АПН: Надо включить в проект файл stm32f10x_flash.c и раскомментировать stm32f10x_flash.h 
      в файле stm32f10x_conf.h
*/

/*
*********************************************************************************************************
*
*                                        BOARD SUPPORT PACKAGE
*
*                                     ST Microelectronics STM32
*                                              with the
*                                   STM3210B-EVAL Evaluation Board
*                         (проект-прототип предполагал использование этой платы)
* Filename      : bsp.c
* Version       : V1.10
* Programmer(s) : BAN     (с комментариями АПН)
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#define  BSP_MODULE
#include <bsp.h>

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/
    /* -------------------- GPIOA PINS -------------------- */
//#define  BSP_GPIOA_PB_WAKEUP                      DEF_BIT_00
#define  BSP_GPIOA_UART1_TX                       DEF_BIT_09
#define  BSP_GPIOA_UART1_RX                       DEF_BIT_10


/*
*********************************************************************************************************
*                                               BSP_Init()
*
* Description : Initialize the Board Support Package (BSP).
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) This function SHOULD be called before any other BSP function is called.
*********************************************************************************************************
*/

void  BSP_Init (void)
{
    RCC_DeInit();               //  Сбрасывает RCC в исходное состояние (после Reset'а)
    RCC_HSEConfig(RCC_HSE_ON);  //  Выбор источника тактирования - 
    RCC_WaitForHSEStartUp();    //  Ожидание установления тактирования


    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK2Config(RCC_HCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);   //  Частота тактирования АЦП <АПН>

#ifdef STM32F10X_MD                     //  <АПН!!>
    FLASH_SetLatency(FLASH_Latency_2);  //  Число циклов ожидания при доступе к Flash   
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);   //  буфер предвыборки      
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);    //  72 МГц=8МГц*9           
#elif  STM32F10X_MD_VL                  //  <АПН!!>
    RCC->CFGR=0x00130000;               //  <АПН!!  это для платы с МК STM32F100RB>
#endif                                  //  <АПН!!>
    RCC_PLLCmd(ENABLE);                 //  Включили ФАПЧ

    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) {   //  Ждет установления
        ;
    }

    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);      //  Тактирование от ФАПЧ

    while (RCC_GetSYSCLKSource() != 0x08) {         //  Ждет подтверждения, что от ФАПЧ
        ;
    }
}

/*
*********************************************************************************************************
*                                            BSP_CPU_ClkFreq()
*
* Description : Read CPU registers to determine the CPU clock frequency of the chip.
*
* Argument(s) : none.
*
* Return(s)   : The CPU clock frequency, in Hz.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT32U  BSP_CPU_ClkFreq (void)
{
    RCC_ClocksTypeDef  rcc_clocks;


    RCC_GetClocksFreq(&rcc_clocks);

    return ((CPU_INT32U)rcc_clocks.HCLK_Frequency);
}

/*
*********************************************************************************************************
*********************************************************************************************************
*                                         OS CORTEX-M3 FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         OS_CPU_SysTickClkFreq()
*
* Description : Get system tick clock frequency.
*
* Argument(s) : none.
*
* Return(s)   : Clock frequency (of system tick).
*
* Caller(s)   : BSP_Init().
*
* Note(s)     : none.
*********************************************************************************************************
*/

INT32U  OS_CPU_SysTickClkFreq (void)
{
    INT32U  freq;


    freq = BSP_CPU_ClkFreq();
    return (freq);
}

