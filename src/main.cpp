#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "pins.h"

#include "switch.h"
#include "soil_moisture.h"
#include "water_level.h"
#include "networking.h"

#include "time.h"

void stuff (void *pvParameter) {
	CapacitiveMoistureSensor soil {SOIL_MOISTURE_SENSOR_ADC_UNIT, SOIL_MOISTURE_SENSOR_ADC_CHANNEL};
	WaterLevelSensor water {WATER_LEVEL_SENSOR_PIN, WATER_LEVEL_EMPTY_LEVEL};

	Switch pump {PUMP_CONTROL_PIN};
	Switch relay {RELAY_CONTROL_PIN};

	init_networking();
	init_wifi();
	init_sntp();

	time_t now;
    struct tm timeinfo;
	char strftime_buf[64];


	while (true) {
		time(&now);
		localtime_r(&now, &timeinfo);
		strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
		printf("Time: %s\n", strftime_buf);
		printf("Water Empty: %i\n", water.IsEmpty());
		soil.measure();
		relay.turn_on();
		vTaskDelay(pdMS_TO_TICKS(500));
		relay.turn_off();
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

extern "C" void app_main() {
	xTaskCreate(&stuff, "stuff", 8192, NULL, 5, NULL);
}