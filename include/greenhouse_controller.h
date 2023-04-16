#ifndef __GREENHOUSE_CONTROLLER_H__
#define __GREENHOUSE_CONTROLLER_H__

#include <time.h>
#include <string>

#include "switch.h"
#include "water_level.h"
#include "soil_moisture.h"

class GreenhouseController {
public:
	GreenhouseController();

	bool receive_message(const std::string &topic, const std::string &data);

private:
	Switch _led_relay;
	Switch _pump;
	WaterLevelSensor _water_level;
	CapacitiveMoistureSensor _soil_moisture;

	bool _override_pump;
	bool _override_pump_value;
	bool _override_led;
	bool _override_led_value;

	int _moisture_threshold;
	unsigned long _led_start;
	unsigned long _led_end;
	
	static void main_loop(void *pvParameter);

	void report();
	void report_state();
	void report_int(int value, const char *topic);

	void control_led();
	void control_pump();

	void time_to_string(unsigned long t, char *buf);
	unsigned long string_to_time(const std::string &str);
};

#endif