#include <ch32v20x.h>
#include "nand.h"

#define JEDEC_ID_COMMAND 0x9F
#define READ_COMMAND 0x3
#define ENABLE_RESET_COMMAND 0x66
#define RESET_DEVICE_COMMAND 0x99
#define WRITE_ENABLE_COMMAND 0x6
#define PAGE_PROGRAM_COMMAND 0x2
#define CHIP_ERASE_COMMAND   0xC7

#define JEDEC_ID 0xEF

enum State state;

static void W25Q_Error_Handler(void){
    SPI1->CTLR2 &= ~SPI_CTLR2_RXNEIE;
    SPI1->CTLR2 |= SPI_CTLR2_TXEIE;
    state = ENABLE_RESET;
    GPIOA->BSHR = GPIO_BSHR_BS4;
}

uint32_t address=0;
static uint8_t address_counter;
static uint16_t counter;
uint8_t buffer[BUFFER_SIZE];

enum W25_State w25_state = Read;

static void Reset_W25Q(void)
{
    switch (state)
    {
    case ENABLE_RESET:
        if((SPI1->STATR & SPI_STATR_TXE) && (SPI1->CTLR2 & SPI_CTLR2_TXEIE))
        {
            GPIOA->BSHR = GPIO_BSHR_BR4;
            uint8_t data = *((__IO uint8_t*)(&SPI1->DATAR));
            *((__IO uint8_t*)(&SPI1->DATAR)) = ENABLE_RESET_COMMAND;
            SPI1->CTLR2 &= ~SPI_CTLR2_TXEIE;
            SPI1->CTLR2 |= SPI_CTLR2_RXNEIE;
        }
        else if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            uint8_t data = *((__IO uint8_t*)(&SPI1->DATAR));
            GPIOA->BSHR = GPIO_BSHR_BS4;
            state = DEVICE_RESET;
            SPI1->CTLR2 &= ~SPI_CTLR2_RXNEIE;
            SPI1->CTLR2 |= SPI_CTLR2_TXEIE;
        }
        break;
    case DEVICE_RESET:
        if((SPI1->STATR & SPI_STATR_TXE) && (SPI1->CTLR2 & SPI_CTLR2_TXEIE))
        {
            GPIOA->BSHR = GPIO_BSHR_BR4;
            uint8_t data = *((__IO uint8_t*)(&SPI1->DATAR));
            *((__IO uint8_t*)(&SPI1->DATAR)) = RESET_DEVICE_COMMAND;
            SPI1->CTLR2 &= ~SPI_CTLR2_TXEIE;
            SPI1->CTLR2 |= SPI_CTLR2_RXNEIE;
        }
        else if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            uint8_t data = *((__IO uint8_t*)(&SPI1->DATAR));
            GPIOA->BSHR = GPIO_BSHR_BS4;
            state = SEND_READ_JEDEC;
            SPI1->CTLR2 &= ~SPI_CTLR2_RXNEIE;
            SPI1->CTLR2 |= SPI_CTLR2_TXEIE;
        }
        break;
    case SEND_READ_JEDEC:
        if((SPI1->STATR & SPI_STATR_TXE) && (SPI1->CTLR2 & SPI_CTLR2_TXEIE))
        {
            GPIOA->BSHR = GPIO_BSHR_BR4;
            uint8_t data = *((__IO uint8_t*)(&SPI1->DATAR));
            *((__IO uint8_t*)(&SPI1->DATAR)) = JEDEC_ID_COMMAND;
            SPI1->CTLR2 &= ~SPI_CTLR2_TXEIE;
            SPI1->CTLR2 |= SPI_CTLR2_RXNEIE;
        }
        else if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            state = ACCEPT_JEDEC_ID;
            uint8_t data = *((__IO uint8_t*)(&SPI1->DATAR));
            *((__IO uint8_t*)(&SPI1->DATAR)) = 0xFF;
        }
        break;
    case ACCEPT_JEDEC_ID:
        if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            uint8_t data = *((__IO uint8_t*)(&SPI1->DATAR));
            if(data != JEDEC_ID)
                W25Q_Error_Handler();
            else
            {
                state = ACCEPT_ID2;
                *((__IO uint8_t*)(&SPI1->DATAR)) = 0xFF;
            }
        }
        break;
    case ACCEPT_ID1:
        if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            uint8_t data = *((__IO uint8_t*)(&SPI1->DATAR));
            *((__IO uint8_t*)(&SPI1->DATAR)) = 0xFF;
            state = ACCEPT_ID2;
        }
        break;
    case ACCEPT_ID2:
        if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            uint8_t data = *((__IO uint8_t*)(&SPI1->DATAR));
            state = FREE;
            SPI1->CTLR2 &= ~SPI_CTLR2_RXNEIE;
            SPI1->CTLR2 |= SPI_CTLR2_TXEIE;
            GPIOA->BSHR = GPIO_BSHR_BS4;
        }
        break;
    default:
        W25Q_Error_Handler();
    }
}

static void SPI_Reader(void)
{
    switch (state)
    {
    case ENABLE_RESET:
    case DEVICE_RESET:
    case SEND_READ_JEDEC:
    case ACCEPT_JEDEC_ID:
    case ACCEPT_ID1:
    case ACCEPT_ID2:
        Reset_W25Q();
        break;
    case FREE:
        if((SPI1->STATR & SPI_STATR_TXE) && (SPI1->CTLR2 & SPI_CTLR2_TXEIE))
        {
            GPIOA->BSHR = GPIO_BSHR_BR4;
            counter=0;
            *((__IO uint8_t*)(&SPI1->DATAR)) = READ_COMMAND;
            state = SEND_ADDRESS;
            address_counter = 0;
        }
        break;
    case SEND_ADDRESS:
        if((SPI1->STATR & SPI_STATR_TXE) && (SPI1->CTLR2 & SPI_CTLR2_TXEIE))
        {
            *((__IO uint8_t*)(&SPI1->DATAR)) = (address>>(8*(address_counter++))) & 0xFF;
            if(address_counter == 3)
            {
                state = READ_DATA;
                uint8_t data = *((__IO uint8_t*)(&SPI1->DATAR));
                SPI1->CTLR2 |= SPI_CTLR2_RXNEIE;
                SPI1->CTLR2 &= ~SPI_CTLR2_TXEIE;
            }
        }
        break;
    case READ_DATA:
        if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            buffer[counter++] = *((__IO uint8_t*)(&SPI1->DATAR));
            if(counter == BUFFER_SIZE)
            {
                state = FREE;
                SPI1->CTLR2 &= ~SPI_CTLR2_RXNEIE;
                GPIOA->BSHR = GPIO_BSHR_BS4;
            }
            else
                *((__IO uint8_t*)(&SPI1->DATAR)) = 0xff;
        }
        break;
    default:
        W25Q_Error_Handler();
        break;
    }
}

static void SPI_Writer(void)
{
    switch (state)
    {
    case ENABLE_RESET:
    case DEVICE_RESET:
    case SEND_READ_JEDEC:
    case ACCEPT_JEDEC_ID:
    case ACCEPT_ID1:
    case ACCEPT_ID2:
        Reset_W25Q();
        break;
    case FREE:
        if((SPI1->STATR & SPI_STATR_TXE) && (SPI1->CTLR2 & SPI_CTLR2_TXEIE))
        {
            GPIOA->BSHR = GPIO_BSHR_BR4;
            *((__IO uint8_t*)(&SPI1->DATAR)) = WRITE_ENABLE_COMMAND;
            SPI1->CTLR2 &= ~SPI_CTLR2_TXEIE;
            SPI1->CTLR2 |= SPI_CTLR2_RXNEIE;
        }
        else if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            state = SEND_WRITE_ENABLE;
            uint8_t data = *((__IO uint8_t*)(&SPI1->DATAR));
            GPIOA->BSHR = GPIO_BSHR_BS4;
            SPI1->CTLR2 &= ~SPI_CTLR2_RXNEIE;
            SPI1->CTLR2 |= SPI_CTLR2_TXEIE;
        }
        break;
    case SEND_WRITE_ENABLE:
        if((SPI1->STATR & SPI_STATR_TXE) && (SPI1->CTLR2 & SPI_CTLR2_TXEIE))
        {
            GPIOA->BSHR = GPIO_BSHR_BR4;
            *((__IO uint8_t*)(&SPI1->DATAR)) = CHIP_ERASE_COMMAND;
            SPI1->CTLR2 &= ~SPI_CTLR2_TXEIE;
            SPI1->CTLR2 |= SPI_CTLR2_RXNEIE;
        }
        else if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            state = ERASED;
            uint8_t data = *((__IO uint8_t*)(&SPI1->DATAR));
            GPIOA->BSHR = GPIO_BSHR_BS4;
            SPI1->CTLR2 &= ~SPI_CTLR2_RXNEIE;
            SPI1->CTLR2 |= SPI_CTLR2_TXEIE;
            counter=0;
        }
        break;
    case ERASED:
        switch(counter)
        {
        case 0:
            if((SPI1->STATR & SPI_STATR_TXE) && (SPI1->CTLR2 & SPI_CTLR2_TXEIE))
            {
                GPIOA->BSHR = GPIO_BSHR_BR4;
                *((__IO uint8_t*)(&SPI1->DATAR)) = PAGE_PROGRAM_COMMAND;
                SPI1->CTLR2 &= ~SPI_CTLR2_TXEIE;
                SPI1->CTLR2 |= SPI_CTLR2_RXNEIE;
            }
            else if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
            {
                uint8_t data = *((__IO uint8_t*)(&SPI1->DATAR));
                SPI1->CTLR2 &= ~SPI_CTLR2_RXNEIE;
                SPI1->CTLR2 |= SPI_CTLR2_TXEIE;
                counter=1;
                address_counter = 0;
            }
            break;
        case 1:
        case 2:
        case 3:
            if((SPI1->STATR & SPI_STATR_TXE) && (SPI1->CTLR2 & SPI_CTLR2_TXEIE))
            {
                *((__IO uint8_t*)(&SPI1->DATAR)) = (address >> ((2-address_counter)*8))&0xff;
                SPI1->CTLR2 &= ~SPI_CTLR2_TXEIE;
                SPI1->CTLR2 |= SPI_CTLR2_RXNEIE;
            }
            else if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
            {
                uint8_t data = *((__IO uint8_t*)(&SPI1->DATAR));
                SPI1->CTLR2 &= ~SPI_CTLR2_RXNEIE;
                SPI1->CTLR2 |= SPI_CTLR2_TXEIE;
                counter++;
                address_counter++;
            }
            break;
        case PAGE_SIZE+4:
            SPI1->CTLR2 &= ~SPI_CTLR2_TXEIE;
            address += 256;
            counter = 0;
            address_counter = 0;
            GPIOA->BSHR = GPIO_BSHR_BS4;
            break;
        default:
            *((__IO uint8_t*)(&SPI1->DATAR)) = buffer[(counter++)-4];
            break;
        }
        break;
    default:
        W25Q_Error_Handler();
        break;
    }
}

__attribute__((interrupt("WCH-Interrupt-fast")))
void SPI1_IRQHandler(void)
{
    switch(w25_state)
    {
    case Read:
        SPI_Reader();
        break;
    case Write:
        SPI_Writer();
        break;
    }
}
