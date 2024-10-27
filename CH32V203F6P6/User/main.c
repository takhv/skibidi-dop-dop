/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : Kozodoy Andrey, Takhvatulin Mikhail, Schetinin Stanislav and Mikhaylov Pavel
 * Version            : V1.0.0
 * Date               : 23.11.2024
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

void setUart()
{
    RCC->APB2PCENR |= RCC_IOPAEN;//PA enable
    while((RCC->APB2PCENR & RCC_IOPAEN) != RCC_IOPAEN);
    GPIOA->CFGLR &= ~(1<<14);
    GPIOA->CFGLR |= 1<<15;

    RCC->APB1PCENR |= RCC_USART2EN;
    USART2->BRR = (2<<4)|(1<<3); // 600.000 baud
    USART2->CTLR1 |= USART_CTLR1_UE | USART_CTLR1_RXNEIE | USART_CTLR1_RE;
}

enum Command
{
    NONE,
    READ,
    SEND
} command;

__attribute__((interrupt("WCH-Interrupt-fast")))
void USART2_IRQHandler(void)
{
    if(USART2->STATR & USART_STATR_RXNE)
    {
        uint8_t data = USART2->DATAR;
        switch(command)
        {
        case NONE:
            if(data == 0x69)
                command = READ;
            else if (data == 0x96)
                command = SEND;
            else if (data == 0x05)
                GPIOA->BSHR = GPIO_BSHR_BR4;
            else if (data == 0x50)
                GPIOA->BSHR = GPIO_BSHR_BS4;
            break;
        case SEND:
            SPI1->DATAR = data;
            break;
        case READ:
            SPI1->CTLR2 |= SPI_CTLR2_RXNEIE;
            SPI1->DATAR = 0;
            break;
        }
    }
}

__attribute__((interrupt("WCH-Interrupt-fast")))
void SPI1_IRQHandler(void)
{
    if((SPI1->STATR & SPI_STATR_TXE) && command == SEND)
    {
        command = NONE;
    }
    if((SPI1->STATR & SPI_STATR_RXNE) && command == READ)
    {
        USART2->DATAR = SPI1->DATAR;
        command = NONE;
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
    GPIOA->CFGLR |= GPIO_CFGLR_MODE4_0; //PA4 out 10MHz
    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF4; //PA4 push-pull

    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF5;
    GPIOA->CFGLR |= GPIO_CFGLR_MODE5_0;
    GPIOA->CFGLR |= GPIO_CFGLR_CNF5_1;//SCK alternate push-pull output

    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF6;
    GPIOA->CFGLR |= GPIO_CFGLR_CNF6_1;//MISO alternate pull-up input

    GPIOA->CFGLR &= ~GPIO_CFGLR_CNF7;
    GPIOA->CFGLR |= GPIO_CFGLR_MODE7_0;
    GPIOA->CFGLR |= GPIO_CFGLR_CNF7_1;//MOSI alternate push-pull output

    RCC->APB2PCENR |= RCC_APB2Periph_SPI1;

    SPI1->CTLR1 |= SPI_CTLR1_MSTR | SPI_CTLR1_SSM;
    SPI1->CTLR2 |= SPI_CTLR2_TXEIE;
    SPI1->CTLR1 |= SPI_CTLR1_SPE;
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
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_EnableIRQ(SPI1_IRQn);
    setClock();
    setUart();
    setSPI();
    GPIOA->BSHR = GPIO_BSHR_BS4;
    while(1)
    {
    }
}
