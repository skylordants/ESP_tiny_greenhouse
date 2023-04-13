#ifndef __SWITCH_H__
#define __SWITCH_H__

#include "driver\gpio.h"

class Switch {
public:
	Switch(int pin);
	Switch();

	void turn_on();
	void turn_off();
	void set_state(bool state);

	bool is_on();

private:
	gpio_config_t _io_config;

	bool _on;
	int _pin;

};

#endif