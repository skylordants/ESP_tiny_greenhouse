#include "greenhouse_controller.h"
#include "pins.h"
#include "networking.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include <time.h>
#include <string.h>
#include <string>

GreenhouseController::GreenhouseController()
	: _led_relay(RELAY_CONTROL_PIN)
	, _pump(PUMP_CONTROL_PIN)
	, _water_level(WATER_LEVEL_SENSOR_PIN, WATER_LEVEL_EMPTY_LEVEL)
	, _soil_moisture(SOIL_MOISTURE_SENSOR_ADC_UNIT, SOIL_MOISTURE_SENSOR_ADC_CHANNEL)
	, _override_pump(false)
	, _override_pump_value(false)
	, _override_led(false)
	, _override_led_value(false)
	, _moisture_threshold(1500)
	, _led_start(8*3600)
	, _led_end(18*3600)
{
	xTaskCreate(main_loop, "greenhouse_loop", 8192, this, 5, NULL);

	mqtt_subscribe("greenhouse/control/#");
	register_greenhouse(this);
	report_state();
}

bool GreenhouseController::receive_message(const std::string &topic, const std::string &data) {
	printf("Topic: %s , data: %s\n", topic.c_str(), data.c_str()); // Use printf and .c_str() instead of std::cout, cause iostream took 20% of flash 
	if (topic.starts_with("override/")) {
		// Override certain controls
		std::string subtopic = {topic, 9};
		if (subtopic == "pump") {
			_override_pump = data[0] == '1';
		} else if (subtopic == "led") {
			_override_led = data[0] == '1';
		} else {
			printf("Unknown greenhouse control topic %s , data %s\n", topic.c_str(), data.c_str());
			return false;
		}
	} else if (topic.starts_with("automatic/")) {
		std::string subtopic {topic, 10};
		// Automatics settings
		if (subtopic == "led_start") {
			long time = string_to_time(data);
			if (time != -1) {
				_led_start = time;
			}
		} else if (subtopic == "led_end") {
			long time = string_to_time(data);
			if (time != -1) {
				_led_end = time;
			}
		} else if (subtopic == "moisture_threshold") {
			_moisture_threshold = std::stoi(data);
		} else {
			printf("Unknown greenhouse control topic %s , data %s\n", topic.c_str(), data.c_str());
			return false;
		}
	} else if (topic == "pump") {
		_override_pump_value = data[0] == '1';
	} else if (topic == "led") {
		_override_led_value = data[0] == '1';
	} else {
		printf("Unknown greenhouse control topic %s , data %s\n", topic.c_str(), data.c_str());
		return false;
	}

	report_state();
	return true;
}


void GreenhouseController::main_loop(void *pvParameter) {
	GreenhouseController *gc = static_cast<GreenhouseController *>(pvParameter);
	while (true) {
		gc->control_led();
		gc->control_pump();

		gc->report();
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void GreenhouseController::report() {
	_soil_moisture.measure();

	// Report all sensor and switch states
	report_int(_soil_moisture.getVoltage(), "greenhouse/measurements/soil_moisture");
	report_int(_water_level.IsFull(), "greenhouse/measurements/water");
	report_int(_pump.is_on(), "greenhouse/measurements/pump");
	report_int(_led_relay.is_on(), "greenhouse/measurements/led");

	// Report time
	char buf[64];
	time_t now;
    struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);
	strftime(buf, sizeof(buf), "%c", &timeinfo);

	mqtt_publish("greenhouse/report_time", buf, 1);
}

void GreenhouseController::report_state() {
	// Report settings state
	report_int(_override_led, "greenhouse/state/override/led");
	report_int(_override_pump, "greenhouse/state/override/pump");
	report_int(_override_led_value, "greenhouse/state/led");
	report_int(_override_pump_value, "greenhouse/state/pump");
	
	report_int(_moisture_threshold, "greenhouse/state/automatic/moisture_threshold");

	char buf[7]; // 6 digits and a null byte
	time_to_string(_led_start, buf);
	mqtt_publish("greenhouse/state/automatic/led_start", buf, 1);
	time_to_string(_led_end, buf);
	mqtt_publish("greenhouse/state/automatic/led_end", buf, 1);
}

void GreenhouseController::report_int(int value, const char *topic) {
	char buf[10];
	sprintf(buf, "%d", value);
	mqtt_publish(topic, buf, 1);
}

void GreenhouseController::control_led() {
	if (_override_led) {
		// Network override
		_led_relay.set_state(_override_led_value);
	}
	else {
		// Automatic based on time
	}
}

void GreenhouseController::control_pump() {
	if (_override_pump) {
		// Network override
		_pump.set_state(_override_pump_value);
	}
	else {
		// Automatic based on soil moisture
	}
}

// Buf must be at least 7 chars long
void GreenhouseController::time_to_string(unsigned long t, char *buf) {
	sprintf(buf, "%02lu%02lu%02lu", t/3600%24, t/60%60, t%60);
}	

unsigned long GreenhouseController::string_to_time(const std::string &str) {
	if (str.size() != 6) {
		printf("Wrong length for time, possibly due to mqtt handling errors");
		return -1;
	}

	long temp = std::stol(str);
	unsigned long t = temp%100 + temp/100%100*60 + temp/10000%24*3600;
	return t;
}
