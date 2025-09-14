#pragma once

#include <device/device.h>
#include <device/driver.h>

#include <stdint.h>

struct lcd_device_ops;

struct lcd_device {
    struct device dev;
    const struct lcd_device_ops *ops;
    struct list_head list;
};

struct lcd_device_ops {
    void (*fill)(struct lcd_device *lcd, uint16_t xsta,uint16_t ysta,uint16_t xend,uint16_t yend,uint16_t color);
    void (*point)(struct lcd_device *lcd, uint16_t x,uint16_t y,uint16_t color);
};

struct lcd_driver {
    struct device_driver drv;
    int (*probe)(struct lcd_device *lcd);
    int (*remove)(struct lcd_device *lcd);
};

#define to_lcd_device(d)    container_of(d, struct lcd_device, dev)

int lcd_device_register(struct lcd_device *lcd);
