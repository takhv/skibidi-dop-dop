#include "registers/spi.h"
#define SET_BIT(REG, BIT) REG |= (1<<BIT)
#define RESET_BIT(REG, BIT) REG &= ~(1<<BIT)
#define GET_BIT(REG, BIT) ((REG>>BIT)&1)