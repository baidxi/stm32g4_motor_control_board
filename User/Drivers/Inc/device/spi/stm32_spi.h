#pragma once

#include <device/spi/spi.h>

struct stm32_spi {
    struct spi_master master;
};