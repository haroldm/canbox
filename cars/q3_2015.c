#include "hw_i2c.h"
#include "hw_usart.h"

// Helper to convert a single nibble to hex
static char nibble_to_hex(uint8_t nibble) {
    return (nibble < 10) ? ('0' + nibble) : ('a' + nibble - 10);
}

// Helper to convert a byte to 2-character hex
static void byte_to_hex(uint8_t byte, char *out) {
    out[0] = nibble_to_hex((byte >> 4) & 0xF);
    out[1] = nibble_to_hex(byte & 0xF);
}


// CAN COMFORT
static void q3_2015_ms_2c3_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.acc = STATE_UNDEF;
		carstate.ign = STATE_UNDEF;
		return;
	}

	/*
	   0001 0000 - 0x2C3 : 10 00 00 00 00 00 00 00 - no Key
	   0000 0001 - 0x2C3 : 01 FF FF FF FF FF FF FF - Key inserted, IGN off
	   0000 0111 - 0x2C3 : 07 FF FF FF FF FF FF FF - Ign on
	   0111 1011 - 0x2C3 : 0B FF FF FF FF FF FF FF - Starter
	*/

	if (msg[0] & 0x01)
		carstate.acc = 1;
	else
		carstate.acc = 0;

	if ((msg[0] & 0x02) == 0x02)
		carstate.ign = 1;
	else
		carstate.ign = 0;
}


static void q3_2015_ms_635_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.illum = STATE_UNDEF;
		return;
	}

	// the logic below is useless. here the audi sends a value between 0 and 100=0x64 that doesn't need scaling
	// the value of conf.illum in conf.c is important.

	// day: msg[1] == 0
	// night: msg[1] == 0x64
	
	// night: between 0x06 and 0x64 

	// char buf[9];
	// buf[0] = 'I';
	// buf[1] = 'L';
	// buf[2] = ' ';
	// byte_to_hex(msg[0], &buf[3]);
	// buf[5] = ' ';
	// byte_to_hex(msg[1], &buf[6]);
	// buf[8] = '\n';
	// hw_usart_write(hw_usart_get(), (uint8_t *)buf, 9);

	if (msg[1] == 0) {
		carstate.illum = 0;
	} else if (msg[1] == 0x64) {
		carstate.illum = 100;
	} else { //legacy
		carstate.illum = scale(msg[1], 0x00, 0x64, 0, 100);
	}
}

static void a3_2011_ms_5c3_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	// TODO: because pressing buttons sends a burst of CAN messages, we should only trigger an UART write
	// and a digital potentiometer change when 0x00 happens between messages
	static uint8_t last_msg = 0;

	char buf[6];
	switch (msg[1]) {
		// case 0x00: // rien
		// 	break;
		case 0x06: // vol up 39 06
			// call send_cmd_resistor(vol up)
			//  send_cmd_resistor is:
			// 		1. setting resistance in the i2c potentiometer
			//      2. turning on mosfet
			//      3. setting flag so the interrupt loop will turn off the mosfet at some point
			if (last_msg == 0) {
				last_msg = 0x06;
				struct i2c_t *i2c = hw_i2c_get();
				uint8_t data_to_write = 64; // example wiper value for AD5246
				hw_i2c_write(i2c->baddr, 0x2E, &data_to_write, 1);
				hw_usart_write(hw_usart_get(), "vol+\n", 5);
			}
			break;
		case 0x07: // vol down 39 07
			hw_usart_write(hw_usart_get(), "vol-\n", 5);
			break;
		case 0xa7: // vol push  3b a7
			hw_usart_write(hw_usart_get(), "vol push\n", 9);
			break;
		case 0x0b: // up 39 0b (tel mode is 3a 02)
			hw_usart_write(hw_usart_get(), "up\n", 3);
			break;
		case 0x0c: // down 39 0c (tel mode is 3a 03)
			hw_usart_write(hw_usart_get(), "down\n", 5);
			break;
		// case 0x08: // push left button 39 08
		case 0x2a: // mic 3c 2a
			hw_usart_write(hw_usart_get(), "2a\n", 3);
			break;
		case 0x01: //mode 39 01 when getting out of tel mode. getting in tel mode is 3a 1c
			hw_usart_write(hw_usart_get(), "mode\n", 5);
			break;
		case 0x00: //39 00 when empty	
			if (last_msg != 0) {
				last_msg = 0;
				struct i2c_t *i2c = hw_i2c_get();
				uint8_t data_to_write = 0; // example wiper value for AD5246
				hw_i2c_write(i2c->baddr, 0x2E, &data_to_write, 1);
			}
			break;
		default:	
			byte_to_hex(msg[0], &buf[0]);
			buf[2] = ' ';
			byte_to_hex(msg[1], &buf[3]);
			buf[5] = '\n';
			hw_usart_write(hw_usart_get(), (uint8_t *)buf, 6);
			break;
	}
}

static void q3_2015_ms_470_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	if (is_timeout(desc)) {

		carstate.fl_door = STATE_UNDEF;
		carstate.fr_door = STATE_UNDEF;
		carstate.rl_door = STATE_UNDEF;
		carstate.rr_door = STATE_UNDEF;
		carstate.bonnet = STATE_UNDEF;
		carstate.tailgate = STATE_UNDEF;

		return;
	}

	// 0x470 : 00 00 24 16 20 00 00 00

	if (carstate.fl_door == 0 && (msg[1] & 0x01) == 1) {
		hw_usart_write(hw_usart_get(), "door open\n", 12);

	} else if (carstate.fl_door == 1 && (msg[1] & 0x01) == 0) {
		hw_usart_write(hw_usart_get(), "door close\n", 12);
	}
	carstate.fl_door  = (msg[1] & 0x01) ? 1 : 0;
	carstate.fr_door  = (msg[1] & 0x02) ? 1 : 0;
	carstate.rl_door  = (msg[1] & 0x04) ? 1 : 0;
	carstate.rr_door  = (msg[1] & 0x08) ? 1 : 0;
	carstate.bonnet   = (msg[1] & 0x10) ? 1 : 0;
	carstate.tailgate = (msg[1] & 0x20) ? 1 : 0; // 60 or 20?
}

static struct msg_desc_t q3_2015_ms[] =
{
	{ 0x2c3,  100, 0, 0, q3_2015_ms_2c3_handler }, // ACC
	// { 0x65F,  200, 0, 0, q3_2015_ms_65F_handler }, // VIN
	// { 0x65D, 1000, 0, 0, q3_2015_ms_65D_handler }, // Odometer
	// { 0x571,  600, 0, 0, q3_2015_ms_571_handler }, // Voltage
	// { 0x470,   50, 0, 0, q3_2015_ms_470_handler }, // Doors
	// { 0x359,  100, 0, 0, q3_2015_ms_359_handler }, // Gear selector
//TODO	// { 0x5BF,  100, 0, 0, q3_2015_ms_5BF_handler }, // Keys
	{ 0x635,  100, 0, 0, q3_2015_ms_635_handler }, // Illum
	{ 0x5c3,  500, 0, 0, a3_2011_ms_5c3_handler }, // media keys
	// { 0x35b,  100, 0, 0, q3_2015_ms_35b_handler }, // Taho
	// { 0x621,  100, 0, 0, q3_2015_ms_621_handler }, // Break
	// { 0x6DA,   50, 0, 0, q3_2015_ms_6DA_handler }, // Parks
	// { 0x3E1,  500, 0, 0, q3_2015_ms_3E1_handler }, // AC
	// { 0x3E3,  500, 0, 0, q3_2015_ms_3E3_handler }, // Seat heating
};

