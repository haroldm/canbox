#include <string.h>

#include "hw.h"
#include "hw_can.h"
#include "car.h"

#define USE_Q3_2015
#define USE_A3_2011

static float scale(float value, float in_min, float in_max, float out_min, float out_max)
{
	return (((value - in_min) * (out_max - out_min)) / (in_max - in_min)) + out_min;
}

#define STATE_UNDEF 0xff
typedef struct car_state_t
{
	enum e_car_t car;
	// uint8_t vin[18];
	uint8_t acc;
	uint8_t ign;
	// uint8_t engine;
	// uint32_t taho;
	// uint32_t speed;
	//in percentages 0% : 100%
	uint8_t illum;
	// uint8_t selector;
	// struct radar_t radar;
	//in percentages -100% : 100%
	// int8_t wheel;

} car_state_t;

static car_state_t carstate =
{
	.car = e_car_nums,
	// .vin = { STATE_UNDEF },
	.acc = STATE_UNDEF,
	.ign = STATE_UNDEF,
	// .engine = STATE_UNDEF,
	// .taho = 0,
	// .speed = 0,
	.illum = STATE_UNDEF,
	// .selector = STATE_UNDEF,
	// .radar = { .state = STATE_UNDEF, },
	// .wheel = 0,

};

typedef struct key_state_t
{
	uint8_t key_volume;
	uint8_t key_prev;
	uint8_t key_next;
	uint8_t key_mode;
	uint8_t key_cont;
	uint8_t key_navi;
	uint8_t key_mici;

	struct key_cb_t * key_cb;
} key_state_t;

static struct key_state_t key_state =
{
	.key_volume = STATE_UNDEF,
	.key_prev = STATE_UNDEF,
	.key_next = STATE_UNDEF,
	.key_mode = STATE_UNDEF,
	.key_cont = STATE_UNDEF,
	.key_navi = STATE_UNDEF,
	.key_mici = STATE_UNDEF,
	.key_cb = 0,
};

struct msg_desc_t;
typedef struct msg_desc_t
{
	uint32_t id;
	uint16_t period;
	uint16_t tick;
	uint32_t num;
	void (*in_handler)(const uint8_t * msg, struct msg_desc_t * desc);
} msg_desc_t;

uint8_t is_timeout(struct msg_desc_t * desc)
{
	if (desc->tick >= (2 * desc->period)) {

		desc->tick = 2 * desc->period;
		return 1;
	}

	return 0;
}

#include "cars/anymsg.c"

#ifdef USE_LR2_2007MY
#include "cars/lr2_2007my.c"
#endif

#ifdef USE_LR2_2013MY
#include "cars/lr2_2013my.c"
#endif

#ifdef USE_XC90_2007MY
#include "cars/xc90_2007my.c"
#endif

#ifdef USE_SKODA_FABIA
#include "cars/skoda_fabia.c"
#endif

#ifdef USE_Q3_2015
#include "cars/q3_2015.c"
#ifdef USE_A3_2011
#include "cars/a3_2011.c"
#endif
#endif


#ifdef USE_TOYOTA_PREMIO_26X
#include "cars/toyota_premio_26x.c"
#endif

enum e_car_t car_get_next_car(void)
{
	int car = carstate.car;

	car++;
	if (car >= e_car_nums)
		car = e_car_anymsg;

	return car;
}

static void in_process(struct can_t * can, uint8_t ticks, struct msg_desc_t * msg_desc, uint8_t desc_num)
{
	uint8_t msgs_num = hw_can_get_msg_nums(can);
	uint32_t all_packs = 0;
	for (uint8_t i = 0; i < msgs_num; i++) {

		struct msg_can_t msg;
		if (!hw_can_get_msg(can, &msg, i))
			continue;

		all_packs += msg.num;

		for (uint32_t j = 0; j < desc_num; j++) {

			struct msg_desc_t * desc = &msg_desc[j];

			//special purpose - any activity on the bus
			if (0 == desc->id) {

				if (desc->in_handler) {

					//last msg
					if (i == (msgs_num - 1)) {

						if (all_packs == desc->num)
							desc->tick += ticks;
						else
							desc->tick = 0;

						desc->num = all_packs;

						desc->in_handler(msg.data, desc);
					}
				}
			}
			else if (msg.id == desc->id) {

				if (desc->in_handler) {

					//no new packs, increase timeout
					if (msg.num == desc->num)
						desc->tick += ticks;
					else
						desc->tick = 0;

					desc->num = msg.num;

					desc->in_handler(msg.data, desc);
				}

				break;
			}
		}
	}
}

void car_init(enum e_car_t car, struct key_cb_t * cb)
{
	carstate.car = car;

	carstate.acc = STATE_UNDEF,
	carstate.ign = STATE_UNDEF,
	carstate.illum = STATE_UNDEF,
	// carstate.selector = STATE_UNDEF,
	// carstate.radar.state = STATE_UNDEF,
	// carstate.wheel = 0,

	// carstate.park_lights = STATE_UNDEF,
	// carstate.near_lights = STATE_UNDEF,
	// carstate.park_break = STATE_UNDEF,

	key_state.key_cb = cb;

	e_speed_t speed = e_speed_125;
	if ((car == e_car_skoda_fabia) || (car == e_car_q3_2015) || (car == e_car_a3_2011))
		speed = e_speed_100;
	else if (car == e_car_toyota_premio_26x)
		speed = e_speed_500;
	hw_can_set_speed(hw_can_get_mscan(), speed);
}

enum e_car_t car_get_car(void)
{
	return carstate.car;
}

void car_process(uint8_t ticks)
{
	struct can_t * can = hw_can_get_mscan();

	switch (carstate.car) {

		case e_car_anymsg:
			in_process(can, ticks, anymsg_desc, sizeof(anymsg_desc)/sizeof(anymsg_desc[0]));
			break;
		case e_car_q3_2015:
#ifdef USE_Q3_2015
			in_process(can, ticks, q3_2015_ms, sizeof(q3_2015_ms)/sizeof(q3_2015_ms[0]));
			break;
#endif
		case e_car_a3_2011:
#ifdef USE_A3_2011
			in_process(can, ticks, a3_2011_ms, sizeof(a3_2011_ms)/sizeof(a3_2011_ms[0]));
			break;
#endif
		default:
			break;
	}
}

uint8_t car_get_acc(void)
{
	if (carstate.acc == STATE_UNDEF)
		return 0;

	return carstate.acc;
}

uint8_t car_get_ign(void)
{
	if (carstate.ign == STATE_UNDEF)
		return 0;

	return carstate.ign;
}

uint8_t car_get_illum(void)
{
	if (carstate.illum == STATE_UNDEF)
		return 0;

	return carstate.illum;
}