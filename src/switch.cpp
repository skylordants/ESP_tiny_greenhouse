#include "switch.h"

Switch::Switch(int pin)
	: _io_config({ .pin_bit_mask=1ull<<pin, .mode=GPIO_MODE_OUTPUT, .pull_up_en=GPIO_PULLUP_DISABLE, .pull_down_en=GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_DISABLE})
	, _on(false)
	, _pin(pin)
{
	ESP_ERROR_CHECK(gpio_config(&_io_config));
}

Switch::Switch() {
	
}

void Switch::turn_on() {
	ESP_ERROR_CHECK(gpio_set_level((gpio_num_t) _pin, 1));
}

void Switch::turn_off() {
	ESP_ERROR_CHECK(gpio_set_level((gpio_num_t) _pin, 0));
}