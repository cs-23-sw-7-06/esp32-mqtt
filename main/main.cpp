#include <stdlib.h>

#include <inttypes.h>
#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <mqtt_client.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_event.h>
#include <nvs.h>
#include <nvs_flash.h>

#include <iostream>

#define DELAY 250 // arbitrary tick delay to make sure stuff is properly initialized before proceeding to the next boot step
#define WIFI_SUCCESS_BIT BIT0
#define WIFI_FAIL_BIT BIT1

uint8_t ssid[32] = "p7-hotspot";
uint8_t password[32] = "";

uint8_t max_retries = 10;
uint8_t retries = 0;

EventGroupHandle_t event_group;



void try_exec(esp_err_t err){
	if(err == ESP_OK) return;

	std::cout << "Error!" << std::endl << "\t\033[38;2;255;0;0m";
	switch (err){
		case ESP_ERR_WIFI_NOT_INIT:
			std::cout << "Wifi is not initialized!";
			break;
		case ESP_ERR_INVALID_ARG:
			std::cout << "Invalid arguments!";
			break;
		case ESP_ERR_NO_MEM:
			std::cout << "Out of memory!";
			break;
		case ESP_ERR_WIFI_CONN:
			std::cout << "Internal WiFi error!";
			break;
		case ESP_FAIL:
			std::cout << "Other error!";
			break;
		case ESP_ERR_NVS_NOT_INITIALIZED:
			std::cout << "NVS not initialized!";
			break;
		case ESP_ERR_NVS_NO_FREE_PAGES:
			std::cout << "NVS Storage has no empty pages!";
			break;
		case ESP_ERR_NOT_FOUND:
			std::cout << "No NVS partition!";
			break;
		default:
			std::cout << "Error not configured: " << err;
			break;
	}
	std::cout << "\033[0m" << std::endl;
}


void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
	std::cout << "event handler called" << std::endl;
	switch(event_id){
		case WIFI_EVENT_STA_START:
			std::cout << "Connecteding!" << std::endl;
			try_exec(esp_wifi_connect());
			break;
		case WIFI_EVENT_STA_DISCONNECTED:
			std::cout << "Disconnected! retrying...";
			printf("%d", retries);
			std::cout << std::endl;
			retries++;
			if(retries <= max_retries){
				try_exec(esp_wifi_connect());
				vTaskDelay(DELAY);
			}
			else{
				xEventGroupSetBits(event_group, WIFI_FAIL_BIT);
			}
			break;
		case IP_EVENT_STA_GOT_IP:
			retries = 0;
			//ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
			std::cout << "Got IP Address: " 
				//<< IP2STR(&event->ip_info.ip) 
				<< std::endl;
			xEventGroupSetBits(event_group, WIFI_SUCCESS_BIT);
			break;
	}
}

extern "C" void app_main(void) {

// ################################ NVS INITIALIZATION ################################
	std::cout << "Initializing NVS partition" << std::endl;
	esp_err_t ret = nvs_flash_init();
	if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		try_exec(nvs_flash_erase());
		try_exec(nvs_flash_init());
	}

	vTaskDelay(DELAY);
// ################################ END NVS INITIALIZATION ################################

// ################################ WIFI INITIALIZATION ################################
	std::cout << "Configuring WiFi" << std::endl;
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();

	uint8_t MAC[6] = {0xb8, 0x8a, 0x60, 0x4d, 0x4b, 0x62};

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = *ssid,
			.threshold = {
				.authmode = WIFI_AUTH_OPEN
			}
		}
	};

	event_group = xEventGroupCreate();
	vTaskDelay(DELAY);

	std::cout << "Initializing WiFi...";
	try_exec(esp_netif_init());

	try_exec(esp_event_loop_create_default());

	esp_netif_t *network = esp_netif_create_default_wifi_sta();

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;

	try_exec(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
	try_exec(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

	try_exec(esp_wifi_init(&wifi_init_config));

	vTaskDelay(DELAY);

	std::cout << "Setting WiFi configuration...";
	try_exec(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

	try_exec(esp_wifi_set_mode(WIFI_MODE_STA));

	std::cout << "Starting WiFi...";
	try_exec(esp_wifi_start());

	EventBits_t bits = xEventGroupWaitBits(event_group, WIFI_SUCCESS_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

	if(bits & WIFI_SUCCESS_BIT){
		std::cout << "Successfully connected to: " << ssid << std::endl;
	}
	else if(bits & WIFI_FAIL_BIT){
		std::cout << "Failed to connect to: " << ssid << std::endl;
	}
	else {
		std::cout << "Unknown error!" << std::endl;
	}

	vTaskDelay(DELAY);

// ################################ END WIFI INITIALIZATION ################################

// ################################ MQTT INITIALIZATION ################################
	/*std::cout << "Configuring MQTT" << std::endl;
	const esp_mqtt_client_config_t mqtt_config = {
		.broker = {
			.address = {
				.uri = "mqtt://10.42.0.1:4000"
			}
		}
	};
	vTaskDelay(DELAY);

	std::cout << "Initializing MQTT" << std::endl;
	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_config);

	//try_exec(esp_mqtt_client_register_event(client, MQTT_EVENT_PUBLISHED, (esp_event_handler_t) mqtt_published_handler, client));
	vTaskDelay(DELAY);

	std::cout << "Starting MQTT";
	try_exec(esp_mqtt_client_start(client));

	vTaskDelay(DELAY);*/
// ################################ END MQTT INITIALIZATION ################################


}