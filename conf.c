#include <inttypes.h>

#include <string.h>
#include "conf.h"
// #include "hw_conf.h"

#define HEADER_SIGN 0xdeadbeef
#define TAIL_SIGN 0xdeadbabe

//#define debug 1

#ifdef debug
// #include <stdio.h>
#include "hw_usart.h"
void mdelay(uint32_t msec);
#endif

#pragma pack(1)
typedef struct conf_t
{
	uint32_t header;

	uint16_t idx;
	uint8_t car;
	uint8_t illum;
	uint16_t rear_delay;
	uint8_t canbox;

	uint8_t align;

	uint32_t tail;
} __attribute__ ((__packed__, aligned(4))) conf_t;
#pragma pack()

struct conf_t conf =
{
	.header = HEADER_SIGN,

	.idx = 0,
	.car = e_car_anymsg,
	.illum = 50,
	.rear_delay = 1500,
	.canbox = e_cb_raise_vw_pq,

	.align = 0,

	.tail = TAIL_SIGN,
};

void conf_write(void)
{
}

void conf_read(void)
{
}

enum e_car_t conf_get_car(void)
{
	// return (enum e_car_t)conf.car;
	return e_car_a3_2011;
}

void conf_set_car(enum e_car_t car)
{
	if (car < e_car_nums)
		conf.car = car;
}

uint8_t conf_get_illum(void)
{
	return conf.illum;
}

void conf_set_illum(uint8_t illum)
{
	if (illum <= 100)
		conf.illum = illum;
}

uint16_t conf_get_rear_delay(void)
{
	return conf.rear_delay;
}

void conf_set_rear_delay(uint16_t rear_delay)
{
	if (rear_delay <= MAX_REAR_DELAY)
		conf.rear_delay = rear_delay;
}

enum e_canbox_t conf_get_canbox(void)
{
	return (enum e_canbox_t)conf.canbox;
}

void conf_set_canbox(enum e_canbox_t cb)
{
	if (cb < e_cb_nums)
		conf.canbox = cb;
}

uint16_t conf_get_idx(void)
{
	return conf.idx;
}

