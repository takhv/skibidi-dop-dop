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
#include "nand.h"
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
    USART2->BRR = 24000000 / 115200;
    USART2->CTLR1 |= USART_CTLR1_UE | USART_CTLR1_RXNEIE | USART_CTLR1_RE;
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

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_EnableIRQ(SPI1_IRQn);
    setClock();
    setUart();
    setSPI();
    while(1)
    {
    }
}
