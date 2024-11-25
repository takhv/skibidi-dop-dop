#include <ch32v20x.h>

#define JEDEC_ID_COMMAND 0x9F
#define READ_COMMAND 0x3
#define ENABLE_RESET_COMMAND 0x66
#define RESET_DEVICE_COMMAND 0x99

#define JEDEC_ID 0xEF

enum State {
    ENABLE_RESET,
    DEVICE_RESET,
    SEND_READ_JEDEC,
    ACCEPT_JEDEC_ID,
    ACCEPT_ID2,
    ACCEPT_ID1,
    FREE,
    READ_DATA,
    SEND_ADDRESS,
    ACCEPT_DATA
};
enum State state;

void NAND_Error_Handler(void){
    SPI1->CTLR2 &= ~SPI_CTLR2_RXNEIE;
    SPI1->CTLR2 |= SPI_CTLR2_TXEIE;
    state = ENABLE_RESET;
    GPIOA->BSHR = GPIO_BSHR_BS4;
}

#define BUFFER_SIZE 1024
uint32_t address=0;
uint8_t address_counter;
uint16_t counter;
uint8_t buffer[BUFFER_SIZE];

__attribute__((interrupt("WCH-Interrupt-fast")))
void SPI1_IRQHandler(void)
{
    switch (state)
    {
    case ENABLE_RESET:
        if((SPI1->STATR & SPI_STATR_TXE) && (SPI1->CTLR2 & SPI_CTLR2_TXEIE))
        {
            GPIOA->BSHR = GPIO_BSHR_BR4;
            (void)SPI1->DATAR;
            SPI1->DATAR = ENABLE_RESET_COMMAND;
            SPI1->CTLR2 &= ~SPI_CTLR2_TXEIE;
            SPI1->CTLR2 |= SPI_CTLR2_RXNEIE;
        }
        else if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            (void)SPI1->DATAR;
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
            (void)SPI1->DATAR;
            SPI1->DATAR = RESET_DEVICE_COMMAND;
            SPI1->CTLR2 &= ~SPI_CTLR2_TXEIE;
            SPI1->CTLR2 |= SPI_CTLR2_RXNEIE;
        }
        else if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            (void)SPI1->DATAR;
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
            (void)SPI1->DATAR;
            SPI1->DATAR = JEDEC_ID_COMMAND;
            SPI1->CTLR2 &= ~SPI_CTLR2_TXEIE;
            SPI1->CTLR2 |= SPI_CTLR2_RXNEIE;
        }
        else if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            state = ACCEPT_JEDEC_ID;
            (void)SPI1->DATAR;
            SPI1->DATAR = 0xFF;
        }
        break;
    case ACCEPT_JEDEC_ID:
        if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
            if(SPI1->DATAR != JEDEC_ID)
                NAND_Error_Handler();
            else
            {
                state = ACCEPT_ID2;
                SPI1->DATAR = 0xFF;
            }
        break;
    case ACCEPT_ID1:
        if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            (void)SPI1->DATAR;
            SPI1->DATAR = 0xFF;
            state = ACCEPT_ID2;
        }
        break;
    case ACCEPT_ID2:
        if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            (void)SPI1->DATAR;
            state = FREE;
            SPI1->CTLR2 &= ~SPI_CTLR2_RXNEIE;
            SPI1->CTLR2 |= SPI_CTLR2_TXEIE;
            GPIOA->BSHR = GPIO_BSHR_BS4;
        }
        break;
    case FREE:
        if((SPI1->STATR & SPI_STATR_TXE) && (SPI1->CTLR2 & SPI_CTLR2_TXEIE))
        {
            GPIOA->BSHR = GPIO_BSHR_BR4;
            counter=0;
            SPI1->DATAR = READ_COMMAND;
            state = SEND_ADDRESS;
            address_counter = 0;
        }
        break;
    case SEND_ADDRESS:
        if((SPI1->STATR & SPI_STATR_TXE) && (SPI1->CTLR2 & SPI_CTLR2_TXEIE))
        {
            SPI1->DATAR = (address>>(8*(address_counter++))) & 0xFF;
            if(address_counter == 3)
            {
                state = READ_DATA;
                (void)SPI1->DATAR;
                SPI1->CTLR2 |= SPI_CTLR2_RXNEIE;
                SPI1->CTLR2 &= ~SPI_CTLR2_TXEIE;
            }
        }
        break;
    case READ_DATA:
        if((SPI1->STATR & SPI_STATR_RXNE) && (SPI1->CTLR2 & SPI_CTLR2_RXNEIE))
        {
            buffer[counter++] = SPI1->DATAR;
            if(counter == BUFFER_SIZE)
            {
                state = FREE;
                SPI1->CTLR2 &= ~SPI_CTLR2_RXNEIE;
                GPIOA->BSHR = GPIO_BSHR_BS4;
            }
            else
                SPI1->DATAR = 0xff;
        }
        break;
    }
}
