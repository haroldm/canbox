#ifndef CAR_H
#define CAR_H

#include <inttypes.h>
#include "conf.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct key_cb_t
{
	void (*mode)(void);
	void (*inc_volume)(uint8_t val);
	void (*dec_volume)(uint8_t val);
	void (*prev)(void);
	void (*next)(void);
	void (*navi)(void);
	void (*cont)(void);
	void (*mici)(void);
} key_cb_t;

void car_init(enum e_car_t car, struct key_cb_t * cb);
void car_process(uint8_t);

enum e_car_t car_get_car(void);
enum e_car_t car_get_next_car(void);

uint8_t car_get_acc(void);
uint8_t car_get_ign(void);
uint8_t car_get_illum(void);


#ifdef __cplusplus
}
#endif

#endif

