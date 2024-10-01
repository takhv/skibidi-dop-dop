#include "registers/spi.h"
#define SET_REGISTER(REG, BIT) REG |= (1<<BIT)
#define RESET_REGISTER(REG, BIT) REG &= ~(1<<BIT)