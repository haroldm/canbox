#ifndef HW_I2C_H
#define HW_I2C_H

#include <inttypes.h>

#include "ring.h"

struct gpio_t {
    uint32_t port;
    uint16_t pin;
};

struct i2c_t {
    uint32_t baddr;
    uint32_t rcc;
    uint32_t irq;
    struct gpio_t scl;
    struct gpio_t sda;
};

struct i2c_t * hw_i2c_get(void);

void hw_i2c_reset(uint32_t i2c);

void hw_i2c_setup(uint32_t i2c, uint32_t pclk_mhz);


int hw_i2c_write(uint32_t i2c, uint8_t addr, uint8_t data);

#endif // HW_I2C_H
