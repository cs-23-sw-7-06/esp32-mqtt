#include "main.h"

#include <inttypes.h>
#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mqtt_client.h>
#include <esp_wifi.h>
#include <nvs.h>
#include <nvs_flash.h>

#include <iostream>

using namespace std;

#define DELAY 250 // arbitrary tick delay to make sure stuff is properly initialized before proceeding to the next boot step

void print_err(esp_err_t err){
	cout << "Error!" << endl << "\t\033[38;2;255;0;0m";
	switch (err){
		case ESP_ERR_WIFI_NOT_INIT:
			cout << "Wifi is not initialized!";
			break;
		case ESP_ERR_INVALID_ARG:
			cout << "Invalid arguments!";
			break;
		case ESP_ERR_NO_MEM:
			cout << "Out of memory!";
			break;
		case ESP_ERR_WIFI_CONN:
			cout << "Internal WiFi error!";
			break;
		case ESP_FAIL:
			cout << "Other error!";
			break;
		case ESP_ERR_NVS_NOT_INITIALIZED:
			cout << "NVS not initialized!";
			break;
		case ESP_ERR_NVS_NO_FREE_PAGES:
			cout << "NVS Storage has no empty pages!";
			break;
		case ESP_ERR_NOT_FOUND:
			cout << "No NVS partition!";
			break;
		default:
			cout << "Error not configured: " << err;
			break;
	}
	cout << "\033[0m" << endl;
}

void print_ok(){
	cout << "\033[38;2;0;255;0m \tSuccess!\033[0m" << endl;
}

void mqtt_published_handler(){
	cout << "published" << endl;
}

extern "C" void app_main(void) {
	esp_err_t err;

// ################################ NVS INITIALIZATION ################################
	cout << "Initializing NVS partition" << endl;
	if(!(err = nvs_flash_init())){
		print_ok();
	} else print_err(err);
	vTaskDelay(DELAY);
// ################################ END NVS INITIALIZATION ################################

// ################################ WIFI INITIALIZATION ################################
	cout << "Configuring WiFi" << endl;
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = "p7-hotspot",
			.scan_method = WIFI_FAST_SCAN
		}
	};
	vTaskDelay(DELAY);

	cout << "Initializing WiFi...";
	if(!(err = esp_wifi_init(&wifi_init_config))){
		print_ok();
	} else print_err(err);
	vTaskDelay(DELAY);

	cout << "Setting WiFi configuration...";
	if(!(err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config))){
		print_ok();
	} else print_err(err);

	cout << "Starting WiFi...";
	if(!(err = esp_wifi_start())){
		print_ok();
	} else print_err(err);
	vTaskDelay(DELAY);

	cout << "Connecting WiFi...";
	if(!(err = esp_wifi_connect())){
		print_ok();
	} else print_err(err);
	vTaskDelay(DELAY);

// ################################ END WIFI INITIALIZATION ################################

// ################################ MQTT INITIALIZATION ################################
	cout << "Configuring MQTT" << endl;
	const esp_mqtt_client_config_t mqtt_config = {
		.broker = {
			.address = {
				.uri = "mqtt://10.42.0.1:4000"
			}
		}
	};
	vTaskDelay(DELAY);

	cout << "Initializing MQTT" << endl;
	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_config);

	esp_mqtt_client_register_event(client, MQTT_EVENT_PUBLISHED, (esp_event_handler_t) mqtt_published_handler, client);
	vTaskDelay(DELAY);

	cout << "Starting MQTT";
	if(!(err = esp_mqtt_client_start(client))){
		cout << "Success!" << endl;
	} else print_err(err);
	vTaskDelay(DELAY);
// ################################ END MQTT INITIALIZATION ################################


}