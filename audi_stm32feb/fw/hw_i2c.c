#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/f1/nvic.h>

#include "hw_i2c.h"

// Adjust I2C peripheral here (I2C2 used as example)
static struct i2c_t i2c2 = {
    .baddr = I2C2,
    .rcc = RCC_I2C2,
    // .scl = GPIO_INIT(B, 10),
    .scl = {GPIOB, GPIO10},
    // .sda = GPIO_INIT(B, 11),
    .sda = {GPIOB, GPIO11},
};

struct i2c_t * hw_i2c_get(void)
{
    return &i2c2;
}

/*
void hw_i2c_setup(struct i2c_t *i2c, uint32_t speed_hz,
                  uint8_t *txbuf, uint32_t txbuflen,
                  uint8_t *rxbuf, uint32_t rxbuflen)
{
    // rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_GPIOB);

	rcc_periph_reset_pulse(RST_I2C2);

    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO_I2C2_SCL | GPIO_I2C2_SDA);
    rcc_periph_clock_enable(RCC_I2C2);
    i2c_peripheral_disable(I2C2);
    // i2c_set_clock_frequency(I2C2, 36); // if APB1 = 36 MHz

    // i2c_set_fast_mode(I2C2);
    // i2c_set_ccr(I2C2, 0x1e);
	// i2c_set_trise(I2C2, 0x0b);

    // // i2c_set_ccr(I2C2, 180);            // for 100kHz
    // // i2c_set_trise(I2C2, 37);           // for 100kHz

    i2c_set_speed(I2C2, i2c_speed_sm_100k, 8);
    i2c_set_standard_mode(I2C2);

	i2c_peripheral_enable(I2C2);
}
// */

void hw_i2c_setup(struct i2c_t *i2c, uint32_t speed_hz,
                  uint8_t *txbuf, uint32_t txbuflen,
                  uint8_t *rxbuf, uint32_t rxbuflen)
{
	/* Enable clocks for I2C2 and AFIO. */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
	rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_I2C2EN);
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_AFIOEN);

	/* Set alternate functions for the SCL and SDA pins of I2C2. */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
		      GPIO_I2C2_SCL | GPIO_I2C2_SDA);

	/* Disable the I2C before changing any configuration. */
	i2c_peripheral_disable(I2C2);

i2c_reset(I2C2);                  // Recommended to clear any stale state
i2c_set_clock_frequency(I2C2, 36); // APB1 in MHz
i2c_set_standard_mode(I2C2);
i2c_set_dutycycle(I2C2, I2C_CCR_DUTY_DIV2); // Standard mode
i2c_disable_dual_addressing_mode(I2C2);
// i2c_set_own_address(I2C2, 0x00, I2C_OAR1_ADDMODE_7BIT);
i2c_disable_ack(I2C2); // Disable ACK if you're only transmitting
i2c_peripheral_enable(I2C2);
}

// i2c_read7_v1 and i2c_write7_v1 from https://libopencm3.org/docs/latest/stm32f2/html/i2c__common__v1_8c_source.html 
void hw_i2c_write(uint32_t i2c, int addr, const uint8_t *data, uint32_t n)
{
    while (I2C_SR2(I2C2) & I2C_SR2_BUSY);
    i2c_send_start(I2C2);
    // while (!(I2C_SR1(I2C2) & I2C_SR1_SB));
    // while (!((I2C_SR1(i2c) & I2C_SR1_SB)
    // & (I2C_SR2(i2c) & (I2C_SR2_MSL | I2C_SR2_BUSY))));

    i2c_send_7bit_address(i2c, addr, I2C_WRITE);
    while (!(I2C_SR1(i2c) & I2C_SR1_ADDR));
    (void)I2C_SR2(i2c); // clear ADDR
    for (uint32_t i = 0; i < n; i++) {
        i2c_send_data(i2c, data[i]);
        while (!(I2C_SR1(i2c) & I2C_SR1_BTF));
    }
    i2c_send_stop(i2c);
}
// void hw_i2c_write(uint32_t i2c, int addr, const uint8_t *data, uint32_t n)
// {
//         while ((I2C_SR2(i2c) & I2C_SR2_BUSY)) {
//         }
 
//         i2c_send_start(i2c);
 
//         /* Wait for the end of the start condition, master mode selected, and BUSY bit set */
//         while ( !( (I2C_SR1(i2c) & I2C_SR1_SB)
//                 && (I2C_SR2(i2c) & I2C_SR2_MSL)
//                 && (I2C_SR2(i2c) & I2C_SR2_BUSY) ));
 
//         i2c_send_7bit_address(i2c, addr, I2C_WRITE);
 
//         /* Waiting for address is transferred. */
//         while (!(I2C_SR1(i2c) & I2C_SR1_ADDR));
 
//         /* Clearing ADDR condition sequence. */
//         (void)I2C_SR2(i2c);
 
//         for (uint32_t i = 0; i < n; i++) {
//                 i2c_send_data(i2c, data[i]);
//                 while (!(I2C_SR1(i2c) & (I2C_SR1_BTF)));
//         }
// }
 
void hw_i2c_read(uint32_t i2c, int addr, uint8_t *res, uint32_t n)
{
        i2c_send_start(i2c);
        i2c_enable_ack(i2c);
 
        /* Wait for the end of the start condition, master mode selected, and BUSY bit set */
        while ( !( (I2C_SR1(i2c) & I2C_SR1_SB)
                && (I2C_SR2(i2c) & I2C_SR2_MSL)
                && (I2C_SR2(i2c) & I2C_SR2_BUSY) ));
 
        i2c_send_7bit_address(i2c, addr, I2C_READ);
 
        /* Waiting for address is transferred. */
        while (!(I2C_SR1(i2c) & I2C_SR1_ADDR));
        /* Clearing ADDR condition sequence. */
        (void)I2C_SR2(i2c);
 
        for (uint32_t i = 0; i < n; ++i) {
                if (i == n - 1) {
                        i2c_disable_ack(i2c);
                }
                while (!(I2C_SR1(i2c) & I2C_SR1_RxNE));
                res[i] = i2c_get_data(i2c);
        }
        i2c_send_stop(i2c);
 
        return;
}

// // Simple blocking write (master transmit)
// int hw_i2c_write(struct i2c_t *i2c, uint8_t addr, const uint8_t *data, uint32_t len)
// {
//     i2c_send_start(i2c->baddr);

//     if (!i2c_send_7bit_address(i2c->baddr, addr, I2C_WRITE)) {
//         i2c_send_stop(i2c->baddr);
//         return -1;
//     }

//     for (uint32_t i = 0; i < len; i++) {
//         if (!i2c_send_data(i2c->baddr, data[i])) {
//             i2c_send_stop(i2c->baddr);
//             return -2;
//         }
//         i2c->tx_cnt++;
//     }

//     i2c_send_stop(i2c->baddr);
//     return 0;
// }

// // Simple blocking read (master receive)
// int hw_i2c_read(struct i2c_t *i2c, uint8_t addr, uint8_t *data, uint32_t len)
// {
//     if (!data)
//         return -1;

//     i2c_send_start(i2c->baddr);

//     if (!i2c_send_7bit_address(i2c->baddr, addr, I2C_READ)) {
//         i2c_send_stop(i2c->baddr);
//         return -2;
//     }

//     for (uint32_t i = 0; i < len; i++) {
//         if (i == (len - 1)) {
//             data[i] = i2c_read_nack(i2c->baddr);
//         } else {
//             data[i] = i2c_read_ack(i2c->baddr);
//         }
//         i2c->rx_cnt++;
//     }

//     i2c_send_stop(i2c->baddr);
//     return 0;
// }
