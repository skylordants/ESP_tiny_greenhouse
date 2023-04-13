#ifndef __GREENHOUSE_CONTROLLER_H__
#define __GREENHOUSE_CONTROLLER_H__

#include "switch.h"
#include "water_level.h"
#include "soil_moisture.h"

class GreenhouseController {
public:
	GreenhouseController();

	bool receive_message(const char *topic, const char *data);

private:
	Switch _led_relay;
	Switch _pump;
	WaterLevelSensor _water_level;
	CapacitiveMoistureSensor _soil_moisture;

	bool _override_pump;
	bool _override_pump_value;
	bool _override_led;
	bool _override_led_value;
	
	static void main_loop(void *pvParameter);

	void report();
	void report_state();
	void report_int(int value, const char *topic);

	void control_led();
	void control_pump();
};

#endif