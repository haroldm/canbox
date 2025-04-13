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

	carstate.illum = scale(msg[1], 0x00, 0x63, 0, 100);
}

a3_2011_ms_5c3_handler

static void a3_2011_ms_5c3_handler(const uint8_t * msg, struct msg_desc_t * desc)
{
	switch (msg[1]) {
		// case 0x00: // rien
		// 	break;
		case 0x06: // vol up 39 06
			// call send_cmd_resistor(vol up)
			//  send_cmd_resistor is:
			// 		1. setting resistance in the i2c potentiometer
			//      2. turning on mosfet
			//      3. setting flag so the interrupt loop will turn off the mosfet at some point
			 
			break;
		case 0x07: // vol down 39 07
			break;
		case 0xa7: // vol push  3b a7
			break;
		case 0x02: // up 3a 02
			break;
		case 0x03: // down 3a 03
			break;
		// case 0x2a: // mic 3c 2a
		// 	break;
		// case 0x01: //mode 39 01
		// 	break;
	}
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

