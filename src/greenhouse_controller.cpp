#include "greenhouse_controller.h"
#include "pins.h"
#include "networking.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include <time.h>
#include <string.h>

GreenhouseController::GreenhouseController()
	: _led_relay(RELAY_CONTROL_PIN)
	, _pump(PUMP_CONTROL_PIN)
	, _water_level(WATER_LEVEL_SENSOR_PIN, WATER_LEVEL_EMPTY_LEVEL)
	, _soil_moisture(SOIL_MOISTURE_SENSOR_ADC_UNIT, SOIL_MOISTURE_SENSOR_ADC_CHANNEL)
	, _override_pump(false)
	, _override_pump_value(false)
	, _override_led(false)
	, _override_led_value(false)
{
	xTaskCreate(main_loop, "greenhouse_loop", 8192, this, 5, NULL);

	mqtt_subscribe("greenhouse/control/#");
	register_greenhouse(this);
}

bool GreenhouseController::receive_message(const char *topic, const char *data) {
	if (strncmp(topic, "override", 8) == 0) {
		// Override certain controls
		if (strcmp(topic+8, "/pump") == 0) {
			_override_pump = data[0] == '1';
		} else if (strcmp(topic+8, "/led") == 0) {
			_override_led = data[0] == '1';
		}
	} else if (strcmp(topic, "pump") == 0) {
		_override_pump_value = data[0] == '1';
	} else if (strcmp(topic, "led") == 0) {
		_override_led_value = data[0] == '1';
	} else {
		printf("Unknown greenhouse topic %s , data %s\n", topic, data);
		return false;
	}
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