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
    struct ring_t tx_ring;
    struct ring_t rx_ring;
    uint32_t rx_cnt;
    uint32_t tx_cnt;
};

struct i2c_t * hw_i2c_get(void);

void hw_i2c_setup(struct i2c_t *i2c, uint32_t speed_hz,
                  uint8_t *txbuf, uint32_t txbuflen,
                  uint8_t *rxbuf, uint32_t rxbuflen);

void hw_i2c_write(uint32_t i2c, int addr, const uint8_t *data, uint32_t n);

void hw_i2c_read(uint32_t i2c, int addr, uint8_t *res, uint32_t n);

void hw_i2c_isr(struct i2c_t *i2c);

#endif // HW_I2C_H
