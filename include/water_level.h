#ifndef __WATER_LEVEL_H__
#define __WATER_LEVEL_H__

#include "driver\gpio.h"

// Simple reed switch water level sensor, connect the switch between Vcc and pin
class WaterLevelSensor {
public:
	/// @param empty_level For my switch, if connected to bottom, then 0, if at top, then 1
	WaterLevelSensor(int pin, uint8_t empty_level);
	WaterLevelSensor();

	bool IsEmpty();
	bool IsFull();

private:
	void measure();

	int _pin;
	gpio_config_t _io_config;

	uint8_t _empty_level;
	uint8_t _measurement;
};

#endif