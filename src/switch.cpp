#include "switch.h"

Switch::Switch(int pin)
	: _io_config({ .pin_bit_mask=1ull<<pin, .mode=GPIO_MODE_OUTPUT, .pull_up_en=GPIO_PULLUP_DISABLE, .pull_down_en=GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_DISABLE})
	, _on(false)
	, _pin(pin)
{
	ESP_ERROR_CHECK(gpio_config(&_io_config));
	turn_off();
}

Switch::Switch() {
	
}

void Switch::turn_on() {
	ESP_ERROR_CHECK(gpio_set_level((gpio_num_t) _pin, 1));
	_on = true;
}

void Switch::turn_off() {
	ESP_ERROR_CHECK(gpio_set_level((gpio_num_t) _pin, 0));
	_on = false;
}

bool Switch::is_on() {
	return _on;
}

void Switch::set_state(bool state) {
	if (is_on() != state) {
		if (state) {
			turn_on();
		}
		else {
			turn_off();
		}
	}
}