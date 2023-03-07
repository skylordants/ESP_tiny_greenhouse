#include "water_level.h"


WaterLevelSensor::WaterLevelSensor(int pin, uint8_t empty_level)
	: _pin(pin)
	, _io_config({ .pin_bit_mask=1ull<<_pin, .mode=GPIO_MODE_INPUT, .pull_up_en=GPIO_PULLUP_DISABLE, .pull_down_en=GPIO_PULLDOWN_ENABLE, .intr_type = GPIO_INTR_DISABLE})
{
	if (empty_level > 1) {
		empty_level = 1;
	}
	_empty_level = empty_level;

	ESP_ERROR_CHECK(gpio_config(&_io_config));
}

void WaterLevelSensor::measure() {
	_measurement = gpio_get_level((gpio_num_t)_pin);
}

bool WaterLevelSensor::IsEmpty() {
	measure();
	return _measurement == _empty_level;
}

bool WaterLevelSensor::IsFull() {
	measure();
	return _measurement != _empty_level;
}