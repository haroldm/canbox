// // #include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "hw_usart.h"
#include "conf.h"
#include "car.h"

static float scale(float value, float in_min, float in_max, float out_min, float out_max)
{
	return (((value - in_min) * (out_max - out_min)) / (in_max - in_min)) + out_min;
}

static uint8_t canbox_checksum(uint8_t * buf, uint8_t len)
{
	uint8_t sum = 0;
	for (uint8_t i = 0; i < len; i++)
		sum += buf[i];

	sum = sum ^ 0xff;

	return sum;
}

static uint8_t canbox_hiworld_checksum(uint8_t * buf, uint8_t len)
{
	uint8_t sum = 0;
	for (uint8_t i = 0; i < len; i++)
		sum += buf[i];

	sum = sum - 1;

	return sum;
}

static void snd_canbox_hiworld_msg(uint8_t type, uint8_t * msg, uint8_t size)
{
	uint8_t buf[5/*header header size type ... chksum*/ + size];
	buf[0] = 0x5a;
	buf[1] = 0xa5;
	buf[2] = size;
	buf[3] = type;
	memcpy(buf + 4, msg, size);
	buf[4 + size] = canbox_hiworld_checksum(buf + 2, size + 2);
	hw_usart_write(hw_usart_get(), buf, sizeof(buf));
}

static void snd_canbox_msg(uint8_t type, uint8_t * msg, uint8_t size)
{
	uint8_t buf[4/*header type size ... chksum*/ + size];
	buf[0] = 0x2e;
	buf[1] = type;
	buf[2] = size;
	memcpy(buf + 3, msg, size);
	buf[3 + size] = canbox_checksum(buf + 1, size + 2);
	hw_usart_write(hw_usart_get(), buf, sizeof(buf));
}

/*
 * +------+
 * | Keys |
 * +------+
*/
void canbox_inc_volume(uint8_t val)
{
	(void)val;

	if ((e_cb_raise_vw_pq == conf_get_canbox()) || (e_cb_raise_vw_mqb == conf_get_canbox())) {

		uint8_t buf[] = { 0x01, 0x01 };
		snd_canbox_msg(0x20, buf, sizeof(buf));

		buf[1] = 0x00;
		snd_canbox_msg(0x20, buf, sizeof(buf));
	}
}

void canbox_dec_volume(uint8_t val)
{
	(void)val;

	if ((e_cb_raise_vw_pq == conf_get_canbox()) || (e_cb_raise_vw_mqb == conf_get_canbox())) {

		uint8_t buf[] = { 0x02, 0x01 };
		snd_canbox_msg(0x20, buf, sizeof(buf));

		buf[1] = 0x00;
		snd_canbox_msg(0x20, buf, sizeof(buf));
	}
}

void canbox_prev(void)
{
	if (e_cb_raise_vw_pq == conf_get_canbox()) {

		uint8_t buf[] = { 0x03, 0x01 };
		snd_canbox_msg(0x20, buf, sizeof(buf));

		buf[1] = 0x00;
		snd_canbox_msg(0x20, buf, sizeof(buf));
	}
	else if (e_cb_raise_vw_mqb == conf_get_canbox()) {

		uint8_t buf[] = { 0x04, 0x01 };
		snd_canbox_msg(0x20, buf, sizeof(buf));

		buf[1] = 0x00;
		snd_canbox_msg(0x20, buf, sizeof(buf));
	}
}

void canbox_next(void)
{
	if (e_cb_raise_vw_pq == conf_get_canbox()) {

		uint8_t buf[] = { 0x04, 0x01 };
		snd_canbox_msg(0x20, buf, sizeof(buf));

		buf[1] = 0x00;
		snd_canbox_msg(0x20, buf, sizeof(buf));
	}
	else if( e_cb_raise_vw_mqb == conf_get_canbox()) {

		uint8_t buf[] = { 0x03, 0x01 };
		snd_canbox_msg(0x20, buf, sizeof(buf));

		buf[1] = 0x00;
		snd_canbox_msg(0x20, buf, sizeof(buf));
	}
}

void canbox_mode(void)
{
	if ((e_cb_raise_vw_pq == conf_get_canbox()) || (e_cb_raise_vw_mqb == conf_get_canbox())) {

		uint8_t buf[] = { 0x0a, 0x01 };
		snd_canbox_msg(0x20, buf, sizeof(buf));

		buf[1] = 0x00;
		snd_canbox_msg(0x20, buf, sizeof(buf));
	}
}

void canbox_cont(void)
{
	if ((e_cb_raise_vw_pq == conf_get_canbox()) || (e_cb_raise_vw_mqb == conf_get_canbox())) {

		uint8_t buf[] = { 0x09, 0x01 };
		snd_canbox_msg(0x20, buf, sizeof(buf));

		buf[1] = 0x00;
		snd_canbox_msg(0x20, buf, sizeof(buf));
	}
}

void canbox_mici(void)
{
	if ((e_cb_raise_vw_pq == conf_get_canbox()) || (e_cb_raise_vw_mqb == conf_get_canbox())) {

		uint8_t buf[] = { 0x0c, 0x01 };
		snd_canbox_msg(0x20, buf, sizeof(buf));

		buf[1] = 0x00;
		snd_canbox_msg(0x20, buf, sizeof(buf));
	}
}

enum rx_state
{
	RX_WAIT_START,
	RX_LEN,
	RX_CMD,
	RX_DATA,
	RX_CRC
};

#define RX_BUFFER_SIZE 32
static uint8_t rx_buffer[RX_BUFFER_SIZE];
static uint8_t rx_idx = 0;
static uint8_t rx_state = RX_WAIT_START;
/*header type size data chksum*/
static void canbox_raise_cmd_process(uint8_t ch)
{
	switch (rx_state) {

		case RX_WAIT_START:

			if (ch != 0x2e)
				break;

			rx_idx = 0;
			rx_buffer[rx_idx++] = ch;
			rx_state = RX_CMD;
			break;

		case RX_CMD:

			rx_buffer[rx_idx++] = ch;
			rx_state = RX_LEN;
			break;

		case RX_LEN:

			rx_buffer[rx_idx++] = ch;
			rx_state = ch ? RX_DATA : RX_CRC;
			break;

		case RX_DATA:

			rx_buffer[rx_idx++] = ch;
			{
				uint8_t len = rx_buffer[2];
				rx_state = ((rx_idx - 2) > len) ? RX_CRC : RX_DATA;
			}
			break;

		case RX_CRC:

			rx_buffer[rx_idx++] = ch;
			rx_state = RX_WAIT_START;

			{
				uint8_t ack = 0xff;
				hw_usart_write(hw_usart_get(), (uint8_t *)&ack, 1);

				char buf[64];
				uint8_t cmd = rx_buffer[1];
				// snprintf(buf, sizeof(buf), "\r\nnew cmd %" PRIx8 "\r\n", cmd);
				hw_usart_write(hw_usart_get(), (uint8_t *)buf, strlen(buf));
			}

			break;
	}

	if (rx_idx > RX_BUFFER_SIZE)
		rx_state = RX_WAIT_START;
}

void canbox_cmd_process(uint8_t ch)
{
	if ((e_cb_raise_vw_pq == conf_get_canbox()) || (e_cb_raise_vw_pq == conf_get_canbox()))
		canbox_raise_cmd_process(ch);
}


