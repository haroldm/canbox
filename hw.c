// #include <stdio.h>
#include <string.h>
#include <errno.h>

#include "hw.h"
#include "hw_clock.h"
#include "hw_can.h"
#include "hw_i2c.h"
#include "hw_tick.h"
#include "hw_usart.h"
// #include "hw_conf.h"

#define cm_enable_interrupts() __asm__ __volatile__ ("cpsie i")
#define cm_disable_interrupts() __asm__ __volatile__ ("cpsid i")

static uint8_t usart_tx_ring_buffer[512];
static uint8_t usart_rx_ring_buffer[32];

uint8_t txbuf[32];
uint8_t rxbuf[32];

void hw_setup(void)
{
	cm_disable_interrupts();

	hw_clock_setup();

	hw_gpio_setup();

	hw_systick_setup();

	hw_usart_setup(hw_usart_get(), 38400, usart_tx_ring_buffer, sizeof(usart_tx_ring_buffer), usart_rx_ring_buffer, sizeof(usart_rx_ring_buffer));

	hw_can_setup(hw_can_get_mscan(), e_speed_100);

	struct i2c_t *i2c = hw_i2c_get();
    hw_i2c_setup(i2c, 100000, txbuf, sizeof(txbuf), rxbuf, sizeof(rxbuf));
	uint8_t data_to_write = 0; // example wiper value for AD5246
	hw_i2c_write(i2c->baddr, 0x2E, &data_to_write, 1);

	// hw_conf_setup();

	cm_enable_interrupts();
}

void hw_sleep(void)
{
	cm_disable_interrupts();

	hw_gpio_disable();

	hw_systick_disable();

	hw_usart_disable(hw_usart_get());

	hw_can_sleep(hw_can_get_mscan());

	cm_enable_interrupts();

	hw_cpu_sleep();
}

