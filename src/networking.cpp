#include "networking.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_sntp.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "mqtt_client.h"


#define ESP_WIFI_SSID      "campusnet"
#define ESP_WIFI_PASS      ""
#define ESP_MAXIMUM_RETRY  5
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN


#define SNTP_SERVER "ee.pool.ntp.org"


#define MQTT_SERVER "mqtt://g4.yt"
#define MQTT_USER "greenhouse"
#define MQTT_PASS "greenhouse"

static const char GREENHOUSE_CONTROL_TOPIC[] = "greenhouse/control/";

static const char *TAG = "NETWORKING";

static GreenhouseController *_gc;

void init_networking() {
	ESP_ERROR_CHECK( nvs_flash_init() );
	ESP_ERROR_CHECK( esp_netif_init() );
	ESP_ERROR_CHECK( esp_event_loop_create_default() );
}



// WIFI AND IP STUFF

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

// Event handler for WiFi and IP events
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < ESP_MAXIMUM_RETRY) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		} else {
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG,"connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

void init_wifi() {
	s_wifi_event_group = xEventGroupCreate();

	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
														ESP_EVENT_ANY_ID,
														&event_handler,
														NULL,
														&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
														IP_EVENT_STA_GOT_IP,
														&event_handler,
														NULL,
														&instance_got_ip));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = ESP_WIFI_SSID,
			.password = ESP_WIFI_PASS,
			.threshold = {.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD},
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start() );

	ESP_LOGI(TAG, "wifi_init_sta finished.");

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
			WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
			pdFALSE,
			pdFALSE,
			portMAX_DELAY);

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */
	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
				 ESP_WIFI_SSID, ESP_WIFI_PASS);
	} else if (bits & WIFI_FAIL_BIT) {
		ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
				 ESP_WIFI_SSID, ESP_WIFI_PASS);
	} else {
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
	}

}


// SNTP STUFF


void init_sntp() {
	ESP_LOGI(TAG, "Initializing and starting SNTP");

	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_init();

	setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
	tzset();
}


// MQTT STUFF

static esp_mqtt_client_handle_t mqtt_client;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
	ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
	esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
	switch (static_cast<esp_mqtt_event_id_t>(event_id)) {
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		break;
	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		break;
	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_PUBLISHED:
		//ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_DATA:
		//ESP_LOGI(TAG, "MQTT_EVENT_DATA");

		// Greenhouse control data topic begins with "greenhouse/control/"
		if (strncmp(event->topic, GREENHOUSE_CONTROL_TOPIC, sizeof(GREENHOUSE_CONTROL_TOPIC)-1) == 0) {
			std::string topic {event->topic+(sizeof(GREENHOUSE_CONTROL_TOPIC)-1), static_cast<unsigned int>(event->topic_len-sizeof(GREENHOUSE_CONTROL_TOPIC)+1)};
			std::string data {event->data, static_cast<unsigned int>(event->data_len)};
			if (_gc != nullptr) {
				_gc->receive_message(topic, data);
			}
			else {
				printf("Greenhouse topic, but no Greenhouse :(");
			}
		}

		//printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
		//printf("DATA=%.*s\r\n", event->data_len, event->data);
		break;
	case MQTT_EVENT_ERROR:
		ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
		if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
			ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
		}
		break;
	default:
		ESP_LOGI(TAG, "Other event id:%d", event->event_id);
		break;
	}
}

void init_mqtt() {
	esp_mqtt_client_config_t mqtt_cfg {};
	mqtt_cfg.broker.address.uri = MQTT_SERVER;
	mqtt_cfg.credentials.username = MQTT_USER;
	mqtt_cfg.credentials.authentication.password = MQTT_PASS;

	mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
	esp_mqtt_client_start(mqtt_client);
}

int mqtt_publish(const char *topic, const char *data, int retain, int qos, int len) {
	int msg_id =  esp_mqtt_client_publish(mqtt_client, topic, data, len, qos, retain);
	//ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
	return msg_id;
}

int mqtt_subscribe(const char *topic, int qos) {
	int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, qos);
	ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
	return msg_id;
}

int mqtt_unsubscribe(const char *topic) {
	int msg_id = esp_mqtt_client_unsubscribe(mqtt_client, topic);
	ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
	return msg_id;
}

void register_greenhouse(GreenhouseController *gc) {
	_gc = gc;
}