#include <stdint.h>
#define SPIx_CTRL1_BIDIMODE 15
#define SPIx_CTRL1_BIDIOE 14
#define SPIx_CTRL1_CRCEN 13
#define SPIx_CTRL1_CRCNEXT 12
#define SPIx_CTRL1_DFF 11
#define SPIx_CTRL1_RXONLY 10
#define SPIx_CTRL1_SSM 9
#define SPIx_CTRL1_SSI 8
#define SPIx_CTRL1_LSBFIRST 7
#define SPIx_CTRL1_SPE 6
#define SPIx_CTRL1_BR2 5
#define SPIx_CTRL1_BR1 4
#define SPIx_CTRL1_BR0 3
#define SPIx_CTRL1_MSTR 2
#define SPIx_CTRL1_CPOL 1
#define SPIx_CTRL1_CPHA 0

#define SPIx_CTRL2_TXEIE 7
#define SPIx_CTRL2_RXNEIE 6
#define SPIx_CTRL2_ERRIE 5
#define SPIx_CTRL2_SSOE 2
#define SPIx_CTRL2_TXDMAEN 1
#define SPIx_CTRL2_RXDMAEN 0

#define SPIx_STATR_BSY 7
#define SPIx_STATR_OVR 6
#define SPIx_STATR_MODF 5
#define SPIx_STATR_CRCERR 4
#define SPIx_STATR_UDR 3
#define SPIx_STATR_CHSID 2
#define SPIx_STATR_TXE 1
#define SPIx_STATR_RXNE 0

#define SPIx_I2S_CFGR 11
#define SPIx_I2S_I2SE 10
#define SPIx_I2S_I2CFG1 9
#define SPIx_I2S_I2CFG0 8
#define SPIx_I2S_PCMSYNC 7
#define SPIx_I2S_I2SSTD1 5
#define SPIx_I2S_I2SSTD0 4
#define SPIx_I2S_CKPOL 3
#define SPIx_I2S_DATLEN1 2
#define SPIx_I2S_DATLEN0 1
#define SPIx_I2S_CHLEN 0

#define SPIx_I2SPR_MCKOE 9
#define SPIx_I2SPR_ODD 8
#define SPIx_I2SPR_I2SDIV 0 // размер регистра 8 бит

#define SPIx_HSCR_HSRXEN2 2
#define SPIx_HSCR_HSRXEN 0

typedef struct
{
    uint16_t CTRL1;
    uint16_t CTRL2;
    uint16_t STATR;
    uint16_t DATAR;
    uint16_t CRCR;
    uint16_t RCRCR;
    uint16_t TCRCR;
    uint16_t I2S_CFGR;
    uint16_t HSCR;
} SPI1_typedef;

typedef struct
{
    uint16_t CTRL1;
    uint16_t CTRL2;
    uint16_t STATR;
    uint16_t DATAR;
    uint16_t CRCR;
    uint16_t RCRCR;
    uint16_t TCRCR;
    uint16_t I2S_CFGR;
    uint16_t I2SPR;
    uint16_t HSCR;
} SPI23_typedef;

#define SPI1 ((SPI1_typedef*)(0x40013000))
#define SPI2 ((SPI23_typedef*)(0x4003800))
#define SPI3 ((SPI23_typedef*)(0x40003C00))