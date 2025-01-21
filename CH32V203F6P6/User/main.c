/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : Kozodoy Andrey, Tsalov Vasiliy, Mikhaylov Pavel, Takhvatulin Mikhail and Schetinin Stanislav.
 * Version            : V1.0.0
 * Date               : 02.12.2024
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) Kozodoy Andrey, Tsalov Vasiliy, Mikhaylov Pavel, Takhvatulin Mikhail and Schetinin Stanislav.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller ch32
 *******************************************************************************/

#include "ch32v20x.h"
#include "nand.h"
#include <stdlib.h>

/* Global typedef */

/* Global define */

/* Global Variable */

extern const uint8_t binary_skibidi_audio[];
extern const uint32_t binary_skibidi_size;

void setTimer()
{
    RCC->APB2PCENR |= RCC_IOPAEN;
    while((RCC->APB2PCENR & RCC_IOPAEN) != RCC_IOPAEN);
    GPIOA->CFGLR &= ~(0b100);
    GPIOA->CFGLR |= (0b01)|(0b10<<2);

    RCC->APB1PCENR |= RCC_TIM2EN;
    while((RCC->APB1PCENR & RCC_TIM2EN) != RCC_TIM2EN);
    TIM2->CHCTLR1 = TIM_OC1M_2 | TIM_OC1M_1;
    TIM2->PSC = 1-1;
    TIM2->ATRLR = 256;
    TIM2->CH1CVR = 128;
    TIM2->CCER |= 1;
    TIM2->CTLR1 |= TIM_CEN;

    RCC->APB1PCENR |= RCC_TIM3EN;
    while((RCC->APB1PCENR & RCC_TIM3EN) != RCC_TIM3EN);
    TIM3->PSC = 1-1;
    TIM3->ATRLR = 2172;
    TIM3->CTLR1 |= TIM_URS;
    TIM3->DMAINTENR |= TIM_UIE;
    TIM3->CTLR1 |= TIM_CEN;
}

void setGPIO(void)
{
    RCC->APB2PCENR |= RCC_IOPAEN | RCC_IOPDEN | RCC_AFIOEN;
    GPIOA->CFGHR &= ~(GPIO_CFGHR_CNF11 | GPIO_CFGHR_CNF12);
    GPIOA->CFGHR |= GPIO_CFGHR_MODE11_0 | GPIO_CFGHR_MODE12_0;
}

void setADC(void)
{
    RCC->CFGR0 &= ~RCC_ADCPRE;
    RCC->CFGR0 |= RCC_ADCPRE_0 | RCC_ADCPRE_1;

    RCC->APB2PCENR |= RCC_ADC1EN | RCC_IOPAEN | RCC_IOPBEN;
    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF1;
    while(!(RCC->APB2PCENR & RCC_ADC1EN));
    ADC1->CTLR1 |= ADC_JEOCIE | ADC_SCAN | ADC_JAUTO;
    ADC1->CTLR2 |= ADC_CONT;
    ADC1->ISQR = ((1-1) << 20) | (1 << 10); // Use ADC for 1st channel (PA1)
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
    GPIOA->CFGLR |= GPIO_CFGLR_CNF3_1; // use PA3 as UART Rx

    GPIOA->CFGLR &= ~GPIO_CFGLR_MODE2;
    GPIOA->CFGLR |= GPIO_CFGLR_MODE2_0;
    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF2;
    GPIOA->CFGLR |= GPIO_CFGLR_CNF2_1; // use PA2 as UART Tx

    RCC->APB1PCENR |= RCC_USART2EN;
    USART2->BRR = 24000000/115200;
    USART2->CTLR1 |= USART_CTLR1_UE | USART_CTLR1_RE | USART_CTLR1_RXNEIE | USART_CTLR1_TE;
}

#define potentialMaxError (50)
#define ADC_MAX (0xfff)

#define MAX_ROTATION_ANGLE_IN_LSB (113)
#define MAX_HEAD_ANGLE_IN_LSB (341)
#define HEAD_ANGLE_BASE (0xc00)

uint16_t potentialForHead = HEAD_ANGLE_BASE;

void headLogic() {
    if(potentialForHead != ((HEAD_ANGLE_BASE + MAX_HEAD_ANGLE_IN_LSB) & 0xfff))
        potentialForHead = (HEAD_ANGLE_BASE + MAX_HEAD_ANGLE_IN_LSB) & 0xfff;
    else
        potentialForHead = ((uint16_t)(HEAD_ANGLE_BASE - MAX_HEAD_ANGLE_IN_LSB)) & 0xfff;
}

uint16_t min(uint16_t a, uint16_t b)
{
    return (a>b)?b:a;
}

void updateHeadServo(uint16_t potential)
{
    uint16_t dist_right = ((ADC_MAX - potential) + potentialForHead)&0xfff;
    uint16_t dist_left = ((unsigned)(potential - potentialForHead))&0xfff;
    if(min(dist_left, dist_right) < potentialMaxError)
    {
        GPIOA->BSHR = GPIO_BSHR_BR11 | GPIO_BSHR_BR12;
    }
    else
    {
        if(dist_right > dist_left)
        {
            GPIOA->BSHR = GPIO_BSHR_BR11;
            GPIOA->BSHR = GPIO_BSHR_BS12;
        }
        else
        {
            GPIOA->BSHR = GPIO_BSHR_BR12;
            GPIOA->BSHR = GPIO_BSHR_BS11;
        }
    }
}

__attribute__((interrupt("WCH-Interrupt-fast")))
void ADC1_2_IRQHandler(void)
{
    ADC1->RDATAR;
    uint16_t r1 = ADC1->IDATAR1;
    ADC1->STATR = 0;
    updateHeadServo(r1);
}

__attribute__((interrupt("WCH-Interrupt-fast")))
void USART2_IRQHandler(void)
{
    if(USART2->STATR & USART_STATR_RXNE)
    {
        uint8_t data = USART2->DATAR;
        USART2->DATAR = data;
        TIM2->CH1CVR = data;
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

uint16_t cur_audio = 0;
uint16_t counter_head = 0;

__attribute__((interrupt("WCH-Interrupt-fast")))
void TIM3_IRQHandler(void)
{
    TIM2->CH1CVR = binary_skibidi_audio[cur_audio++];
    if(cur_audio == binary_skibidi_size) {
        cur_audio = 0;
        counter_head ++;
        if(counter_head == 1) {
            counter_head = 0;
            headLogic();
        }
    }
    TIM3->INTFR &= ~1;
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
    setClock();
    setTimer();
    setADC();
//    setUart();
    setGPIO();
    NVIC_EnableIRQ(ADC1_2_IRQn);
//    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_EnableIRQ(TIM3_IRQn);
    while(1)
    {
    }
}
