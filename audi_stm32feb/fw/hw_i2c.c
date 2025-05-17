#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/f1/nvic.h>

#include "hw_i2c.h"
#include "ring.h"

// Adjust I2C peripheral here (I2C2 used as example)
static struct i2c_t i2c2 = {
    .baddr = I2C2,
    .rcc = RCC_I2C2,
    .irq = NVIC_I2C2_EV_IRQ,
    .scl = {GPIOB, GPIO10},
    .sda = {GPIOB, GPIO11},
    .rx_cnt = 0,
    .tx_cnt = 0,
};

struct i2c_t * hw_i2c_get(void)
{
    return &i2c2;
}

void hw_i2c_setup(struct i2c_t *i2c, uint32_t speed_hz,
                  uint8_t *txbuf, uint32_t txbuflen,
                  uint8_t *rxbuf, uint32_t rxbuflen)
{
    // Init ring buffers
    ring_init(&i2c->tx_ring, txbuf, txbuflen);
    ring_init(&i2c->rx_ring, rxbuf, rxbuflen);

    // Enable clocks
    rcc_periph_clock_enable(i2c->rcc);
    rcc_periph_clock_enable(RCC_GPIOB);

    // Setup GPIO pins as AF open-drain
    gpio_set_mode(i2c->scl.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, i2c->scl.pin);
    gpio_set_mode(i2c->sda.port, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, i2c->sda.pin);

    // Reset I2C peripheral
    i2c_peripheral_disable(i2c->baddr);
    i2c_reset(i2c->baddr);

    // Setup speed: you need to set your APB1 clock frequency here
    // extern uint32_t rcc_apb1_frequency; // declare externally or define yourself
    i2c_set_clock_frequency(I2C2, rcc_apb1_frequency / 1000000); // Value in MHz

    // Enable peripheral
    i2c_peripheral_enable(i2c->baddr);

    // // Enable I2C event interrupt (optional, you can poll or do blocking)
    // nvic_enable_irq(i2c->irq);
    // i2c_enable_irq(i2c->baddr, I2C_CR2_ITEVTEN);

    // Reset counts
    i2c->rx_cnt = 0;
    i2c->tx_cnt = 0;
}

// i2c_read7_v1 and i2c_write7_v1 from https://libopencm3.org/docs/latest/stm32f2/html/i2c__common__v1_8c_source.html 
void hw_i2c_write(uint32_t i2c, int addr, const uint8_t *data, uint32_t n)
{
        while ((I2C_SR2(i2c) & I2C_SR2_BUSY)) {
        }
 
        i2c_send_start(i2c);
 
        /* Wait for the end of the start condition, master mode selected, and BUSY bit set */
        while ( !( (I2C_SR1(i2c) & I2C_SR1_SB)
                && (I2C_SR2(i2c) & I2C_SR2_MSL)
                && (I2C_SR2(i2c) & I2C_SR2_BUSY) ));
 
        i2c_send_7bit_address(i2c, addr, I2C_WRITE);
 
        /* Waiting for address is transferred. */
        while (!(I2C_SR1(i2c) & I2C_SR1_ADDR));
 
        /* Clearing ADDR condition sequence. */
        (void)I2C_SR2(i2c);
 
        for (uint32_t i = 0; i < n; i++) {
                i2c_send_data(i2c, data[i]);
                while (!(I2C_SR1(i2c) & (I2C_SR1_BTF)));
        }
}
 
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
