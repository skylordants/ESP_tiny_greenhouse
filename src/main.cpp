#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "soil_moisture.h"
#include "water_level.h"

void stuff (void *pvParameter) {
	CapacitiveMoistureSensor soil {ADC_UNIT_1, ADC_CHANNEL_3};
	WaterLevelSensor water {6, 0};

	while (true) {
		printf("Water Empty: %i\n", water.IsEmpty());
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

extern "C" void app_main() {
	xTaskCreate(&stuff, "stuff", 8192, NULL, 5, NULL);
}