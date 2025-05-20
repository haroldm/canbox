#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/f1/nvic.h>

#include "hw_i2c.h"
#include "hw_usart.h"

#define HW_I2C_TIMEOUT 10000000

static char nibble_to_hex(uint8_t nibble) {
    return (nibble < 10) ? ('0' + nibble) : ('a' + nibble - 10);
}
static void byte_to_hex(uint8_t byte, char *out) {
    out[0] = nibble_to_hex((byte >> 4) & 0xF);
    out[1] = nibble_to_hex(byte & 0xF);
}

// Adjust I2C peripheral here (I2C1 used as example)
static struct i2c_t i2c2 = {
    .baddr = I2C1,
    .rcc = RCC_I2C1,
    // .scl = GPIO_INIT(B, 10),
    // .scl = {GPIOB, GPIO10},
    .scl = {GPIOB, GPIO6},
    // .sda = GPIO_INIT(B, 11),
    // .sda = {GPIOB, GPIO11},
    .sda = {GPIOB, GPIO7},
};

struct i2c_t * hw_i2c_get(void)
{
    return &i2c2;
}
void hw_i2c_reset(uint32_t i2c) {
    I2C_CR1(i2c) |= I2C_CR1_SWRST;
    I2C_CR1(i2c) &= ~I2C_CR1_SWRST;
}

void hw_i2c_setup(uint32_t i2c, uint32_t pclk_mhz) {

    rcc_periph_clock_enable(RCC_GPIOB);

    // Configure PB6 (SCL) and PB7 (SDA) as AF open-drain
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO6);
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO7);

    rcc_periph_clock_enable(RCC_I2C1);

    // Reset and disable I2C1 before configuring
    i2c_peripheral_disable(I2C1);
    i2c_reset(I2C1);

    // Set APB1 clock frequency in MHz (usually 36MHz on STM32F1)
    i2c_set_clock_frequency(I2C1, 36);

    // Set standard mode (100kHz) timings
    i2c_set_standard_mode(I2C1);
    i2c_set_ccr(I2C1, 180);  // 36MHz / (2 * 100kHz)
    i2c_set_trise(I2C1, 37); // 36MHz + 1

    // Enable ACK (optional)
    i2c_enable_ack(I2C1);

    // Set own address (not used in master mode, but required)
    i2c_set_own_7bit_slave_address(I2C1, 0x00);

    // Enable I2C1
    i2c_peripheral_enable(I2C1);

}

static uint32_t dummy_reg;

static int hw_i2c_start(uint32_t i2c, uint8_t addr, uint8_t direction) {
    uint32_t timeout = HW_I2C_TIMEOUT;

    // hw_usart_write(hw_usart_get(), "before send start\n", 18);

    i2c_send_start(i2c);
    // hw_usart_write(hw_usart_get(), "after send start\n", 17);
    while (!(I2C_SR1(i2c) & I2C_SR1_SB))
        if (--timeout == 0) return -1;

    // hw_usart_write(hw_usart_get(), "after device ready\n", 19);

    i2c_send_7bit_address(i2c, addr, direction);
    timeout = HW_I2C_TIMEOUT;
    while (!(I2C_SR1(i2c) & I2C_SR1_ADDR))
        if (--timeout == 0) return -2;

    dummy_reg = I2C_SR2(i2c); // clear ADDR
    return 0;
}

static int hw_i2c_write_byte(uint32_t i2c, uint8_t byte) {
    uint32_t timeout = HW_I2C_TIMEOUT;
    while (!(I2C_SR1(i2c) & I2C_SR1_TxE))
        if (--timeout == 0) return -1;

    i2c_send_data(i2c, byte);
    return 0;
}

static int hw_i2c_stop(uint32_t i2c) {
    uint32_t timeout = HW_I2C_TIMEOUT;
    while (!(I2C_SR1(i2c) & I2C_SR1_BTF))
        if (--timeout == 0) return -1;

    i2c_send_stop(i2c);
    return 0;
}

int hw_i2c_write(uint32_t i2c, uint8_t addr, uint8_t data) {
    hw_usart_write(hw_usart_get(), "writing ", 8);
    uint8_t buf[2] = {0};
    byte_to_hex(data, buf);
    hw_usart_write(hw_usart_get(), buf, 2);
    hw_usart_write(hw_usart_get(), " to device\n ", 11);

    if (hw_i2c_start(i2c, addr, I2C_WRITE)) return -1;
    if (hw_i2c_write_byte(i2c, data))       return -3;
    if (hw_i2c_stop(i2c))                   return -4;
    return 0;
}
