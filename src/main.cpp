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
#include "greenhouse_controller.h"

GreenhouseController *greenhouse;

void init (void *pvParameter) {

	init_networking();
	init_wifi();
	init_sntp();
	init_mqtt();

	greenhouse = new GreenhouseController();

	vTaskDelete(NULL);
}

extern "C" void app_main() {
	xTaskCreate(&init, "init", 8192, NULL, 5, NULL);
}