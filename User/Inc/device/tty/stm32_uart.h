#pragma once

#ifdef STM32G4
#include <stm32g4xx.h>
#include <stm32g4xx_hal.h>
#include <stm32g4xx_hal_uart.h>
#endif

#define TTY_MODE_CONSOLE    1 << 0
#define TTY_MODE_STREAM     1 << 1