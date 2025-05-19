// #include <stdio.h>
#include <string.h>
#include <errno.h>

#include <libopencm3/stm32/i2c.h>

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

	hw_i2c_reset(I2C2);
	hw_i2c_setup(I2C2, 36); // If APB1 = 36MHz
	hw_usart_write(hw_usart_get(), "testst\n", 8);

	for (volatile int i = 0; i < 100000; i++); // crude delay (~few ms depending on clock)
	hw_usart_write(hw_usart_get(), "testes\n", 8);
	int ret = hw_i2c_write(I2C1, 0x2E, 0x32); // Write 0x55 to register 0x00 of slave 0x2C
	if (ret == -1) {
		hw_usart_write(hw_usart_get(), "fail -1\n", 8);
	}
	if (ret == -3) {
		hw_usart_write(hw_usart_get(), "fail -3\n", 8);
	}
	if (ret == -4) {
		hw_usart_write(hw_usart_get(), "fail -4\n", 8);
	}
	ret = hw_i2c_write(I2C1, 0x2E, 0x0); // Write 0x55 to register 0x00 of slave 0x2C

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

