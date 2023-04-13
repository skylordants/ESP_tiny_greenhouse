#ifndef __NETWORKING_H__
#define __NETWORKING_H__

#include "greenhouse_controller.h"

void init_networking();
void init_wifi();
void init_sntp();
void init_mqtt();

int mqtt_publish(const char *topic, const char *data, int retain = 0, int qos = 1, int len = 0);
int mqtt_subscribe(const char *topic, int qos = 1);
int mqtt_unsubscribe(const char *topic);

void register_greenhouse(GreenhouseController *gc);

#endif