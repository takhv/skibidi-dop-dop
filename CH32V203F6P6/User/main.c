/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : Kozodoy Andrey, Takhvatulin Mikhail, Schetinin Stanislav and Mikhaylov Pavel
 * Version            : V1.0.0
 * Date               : 02.12.2024
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) Kozodoy Andrey, Takhvatulin Mikhail, Schetinin Stanislav and Mikhaylov Pavel.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller ch32
 *******************************************************************************/

#include "ch32v20x.h"
#include "nand.h"
#include <stdlib.h>

/* Global typedef */

/* Global define */

/* Global Variable */

void setTimer()
{
    RCC->APB2PCENR |= RCC_IOPAEN;
    while((RCC->APB2PCENR & RCC_IOPAEN) != RCC_IOPAEN);
    GPIOA->CFGLR &= ~(0b100);
    GPIOA->CFGLR |= (0b01)|(0b10<<2);

    RCC->APB1PCENR |= RCC_TIM2EN;
    while((RCC->APB1PCENR & RCC_TIM2EN) != RCC_TIM2EN);
    TIM2->CHCTLR1 = (0b110<<4);
    TIM2->PSC = 1-1;
    TIM2->ATRLR = 256;
    TIM2->CH1CVR = 64;
    TIM2->CCER |= 1;
    TIM2->CTLR1 |= TIM_CEN;

    RCC->APB1PCENR |= RCC_TIM3EN;
    while((RCC->APB1PCENR & RCC_TIM3EN) != RCC_TIM3EN);
    TIM3->PSC = 545-1; // PSC 544 + 96CC1/441ATRLR
    TIM3->ATRLR = 1;
    TIM3->CTLR1 |= TIM_URS;
    TIM3->DMAINTENR |= TIM_UIE;
    TIM3->CTLR1 |= TIM_CEN;
}

void setGPIO(void)
{
    RCC->APB2PCENR |= RCC_IOPAEN | RCC_IOPDEN | RCC_AFIOEN;
    GPIOA->CFGHR &= ~(GPIO_CFGHR_CNF11 | GPIO_CFGHR_CNF12);
    GPIOA->CFGHR |= GPIO_CFGHR_MODE11_0 | GPIO_CFGHR_MODE12_0;

    AFIO->PCFR1 |= AFIO_PCFR1_PD01_REMAP;
    GPIOD->CFGLR &= ~(GPIO_CFGLR_CNF0 | GPIO_CFGLR_CNF1);
    GPIOD->CFGLR |= GPIO_CFGLR_MODE0_0 | GPIO_CFGLR_MODE1_0;
}

void setADC(void)
{
    RCC->CFGR0 &= ~RCC_ADCPRE;
    RCC->APB2PCENR |= RCC_ADC1EN | RCC_IOPAEN | RCC_IOPBEN;
    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF1;
    GPIOB->CFGLR &= ~GPIO_CFGLR_CNF1;
    while(!(RCC->APB2PCENR & RCC_ADC1EN));
    ADC1->CTLR1 |= ADC_JEOCIE | ADC_SCAN | ADC_JAUTO;
    ADC1->CTLR2 |= ADC_CONT;
    ADC1->ISQR = ((2-1) << 20) | (1 << 10) | (9<<15); // 扭把我快技 忱忘扶扶抑抒 扼 忱志批抒 抗忘扶忘抖抉志: 1 我 9 (PA1 我 PB1)
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
    GPIOA->CFGLR |= GPIO_CFGLR_CNF3_1; // 妤我扶 PA3 忱抖攸 UART Rx

    GPIOA->CFGLR |= ~GPIO_CFGLR_MODE2;
    GPIOA->CFGLR |= GPIO_CFGLR_MODE2_0;
    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF2;
    GPIOA->CFGLR |= GPIO_CFGLR_CNF2_1; // 妤我扶 PA2 忱抖攸 UART Tx

    RCC->APB1PCENR |= RCC_USART2EN;
    USART2->BRR = 8000000/115200;
    USART2->CTLR1 |= USART_CTLR1_UE | USART_CTLR1_RE | USART_CTLR1_RXNEIE;
}

int16_t potentialForHead = 0;
int16_t potentialForRotation = 0;
#define ADC_MAX (0xfff)

void hugeHorseDickhead(){
    potentialForHead = (rand() % 21) - 10;
}

void updateHeadServo(int16_t potential)
{
    if((ADC_MAX - potential) + potentialForHead < potential - potentialForHead)
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

void updateRotationServo(int16_t potential)
{
    if((ADC_MAX - potential) + potentialForRotation < potential - potentialForRotation)
    {
        GPIOD->BSHR = GPIO_BSHR_BR1;
        GPIOD->BSHR = GPIO_BSHR_BS0;
    }
    else
    {
        GPIOD->BSHR = GPIO_BSHR_BR0;
        GPIOD->BSHR = GPIO_BSHR_BS1;
    }
}

__attribute__((interrupt("WCH-Interrupt-fast")))
void ADC1_2_IRQHandler(void)
{
    ADC1->RDATAR;
    int16_t r1 = ADC1->IDATAR1;
    int16_t r2 = ADC1->IDATAR2;
    ADC1->STATR &= ~(ADC_EOC | ADC_JEOC);
    updateHeadServo(r1);
    updateRotationServo(r2);
}

static uint32_t counter = 0;

extern enum W25_State w25_state;
extern enum State state;
extern uint8_t buffer[BUFFER_SIZE];
extern uint32_t address;

__attribute__((interrupt("WCH-Interrupt-fast")))
void USART2_IRQHandler(void)
{
    switch(counter)
    {
    case 0:
        if(USART2->DATAR == 0xaa)
        {
            counter = 1;
            USART2->DATAR = 0xaa;
        }
        if(USART2->DATAR == 0x87)
            state = FREE;
        break;
    case 1:
        if(USART2->DATAR == 0x69)
        {
            counter = 2;
            SPI1->CTLR2 &= ~(SPI_CTLR2_RXNEIE | SPI_CTLR2_TXEIE);
            w25_state = Write;
            USART2->DATAR = 0x96;
        }
        else
            counter = 0;
        break;
    case 2:
        address = 0;
    case 3:
    case 4:
        address |= (USART2->DATAR << ((2-(counter-2))*8));
        counter++;
        break;
    default:
        if(counter - 4 < PAGE_SIZE)
        {
            buffer[counter-4] = USART2->DATAR;
            counter++;
        }
        else
        {
            counter=0;
            SPI1->CTLR2 |= SPI_CTLR2_TXEIE;
        }
        break;
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

void setSPI()
{
    RCC->APB2PCENR |= RCC_IOPAEN;//PA enable
    while((RCC->APB2PCENR & RCC_IOPAEN) != RCC_IOPAEN);
    GPIOA->CFGLR |= GPIO_CFGLR_MODE4_0 | GPIO_CFGLR_MODE4_1; //PA4 out 50MHz
    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF4; //PA4 push-pull
    GPIOA->BSHR = GPIO_BSHR_BS4;

    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF5;
    GPIOA->CFGLR |= GPIO_CFGLR_MODE5_0;
    GPIOA->CFGLR |= GPIO_CFGLR_CNF5_1;//SCK alternate push-pull output

    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF6;
    GPIOA->CFGLR |= GPIO_CFGLR_CNF6_1;//MISO alternate pull-up input

    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF7;
    GPIOA->CFGLR |= GPIO_CFGLR_MODE7_0;
    GPIOA->CFGLR |= GPIO_CFGLR_CNF7_1;//MOSI alternate push-pull output

    RCC->APB2PCENR |= RCC_SPI1EN;

    SPI1->CTLR1 |= SPI_CTLR1_MSTR | SPI_CTLR1_SSM | SPI_CTLR1_SSI;
    SPI1->CTLR2 |= SPI_CTLR2_TXEIE;
    SPI1->CTLR1 |= SPI_CTLR1_SPE;
}

uint16_t cur_audio = 0;

__attribute__((interrupt("WCH-Interrupt-fast")))
void TIM3_IRQHandler(void)
{
    TIM2->CH1CVR = buffer[cur_audio++];
    if(cur_audio == BUFFER_SIZE && w25_state == Read)
    {
        SPI1->CTLR2 |= SPI_CTLR2_TXEIE;
        cur_audio %= BUFFER_SIZE;
    }
    else
    {
        TIM2->CH1CVR = 0;
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
    setUart();
    setGPIO();
    setSPI();
    NVIC_EnableIRQ(ADC1_2_IRQn);
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_EnableIRQ(SPI1_IRQn);
    while(1)
    {
    }
}
