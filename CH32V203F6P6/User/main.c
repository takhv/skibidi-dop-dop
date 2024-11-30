/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : Kozodoy Andrey, Takhvatulin Mikhail, Schetinin Stanislav and Mikhaylov Pavel
 * Version            : V1.0.0
 * Date               : 28.11.2024
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) Kozodoy Andrey, Takhvatulin Mikhail, Schetinin Stanislav and Mikhaylov Pavel.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller ch32
 *******************************************************************************/

#include "ch32v20x.h"
/* Global typedef */

/* Global define */

/* Global Variable */

void setADC(void)
{
    RCC->CFGR0 &= ~RCC_ADCPRE;
    RCC->APB2PCENR |= RCC_ADC1EN | RCC_IOPAEN | RCC_IOPBEN;
    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF1;
    GPIOB->CFGLR &= ~GPIO_CFGLR_CNF1;
    while(!(RCC->APB2PCENR & RCC_ADC1EN));
    ADC1->CTLR1 |= ADC_JEOCIE | ADC_SCAN | ADC_JAUTO;
    ADC1->CTLR2 |= ADC_CONT;
    ADC1->ISQR = ((2-1) << 20) | (1 << 10) | (9<<15); // §á§â§Ú§Ö§Þ §Õ§Ñ§ß§ß§í§ç §ã §Õ§Ó§å§ç §Ü§Ñ§ß§Ñ§Ý§à§Ó: 1 §Ú 9 (PA1 §Ú PB1)
    ADC1->CTLR2 |= ADC_JSWSTART;
    ADC1->CTLR2 |= ADC_ADON;
    for(int i = 0;i<10000;i++);
    ADC1->CTLR2 |= ADC_ADON;
}

void setUart()
{
    RCC->APB2PCENR |= RCC_IOPAEN;
    while((RCC->APB2PCENR & RCC_IOPAEN) != RCC_IOPAEN);
    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF3;
    GPIOA->CFGLR |= GPIO_CFGLR_CNF3_1; // §±§Ú§ß PA3 §Õ§Ý§ñ UART Rx

    GPIOA->CFGLR |= ~GPIO_CFGLR_MODE2;
    GPIOA->CFGLR |= GPIO_CFGLR_MODE2_0;
    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF2;
    GPIOA->CFGLR |= GPIO_CFGLR_CNF2_1; // §±§Ú§ß PA2 §Õ§Ý§ñ UART Tx

    RCC->APB1PCENR |= RCC_USART2EN;
    USART2->BRR = 8000000/115200;
    USART2->CTLR1 |= USART_CTLR1_UE | USART_CTLR1_TE;
}

uint32_t counter=0;

__attribute__((interrupt("WCH-Interrupt-fast")))
void ADC1_2_IRQHandler(void)
{
    uint16_t r1 = ADC1->IDATAR1;
    uint16_t r2 = ADC1->IDATAR2;
    if(counter++ == 10000)
    {
        USART2->DATAR = (r1>>8)&0xff;
        counter=0;
    }

}

void setClock()
{
    RCC->CFGR0 |= 1<<18; // pll = 3*hsi
    while((RCC->CFGR0 & (1<<18)) != (1<<18));
    EXTEN->EXTEN_CTR |= EXTEN_PLL_HSI_PRE;
    RCC->CTLR |= 1<<24; // pllon
        while((RCC->CTLR & (1<<24)) != (1<<24));
    RCC->CFGR0 |= 2; // pll for sysclk
    while((RCC->CFGR0 & (2)) != (2));
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
//    setClock();
    setADC();
    setUart();
    NVIC_EnableIRQ(ADC1_2_IRQn);
    while(1)
    {
    }
}
