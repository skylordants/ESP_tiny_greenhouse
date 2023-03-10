#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "pins.h"

#include "switch.h"
#include "soil_moisture.h"
#include "water_level.h"

void stuff (void *pvParameter) {
	CapacitiveMoistureSensor soil {SOIL_MOISTURE_SENSOR_ADC_UNIT, SOIL_MOISTURE_SENSOR_ADC_CHANNEL};
	WaterLevelSensor water {WATER_LEVEL_SENSOR_PIN, WATER_LEVEL_EMPTY_LEVEL};

	Switch pump {PUMP_CONTROL_PIN};
	Switch relay {RELAY_CONTROL_PIN};

	while (true) {
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