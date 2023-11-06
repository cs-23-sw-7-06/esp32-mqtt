#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include <mqtt_client.h>
#include <mqtt5_client.h>
#include <mqtt_supported_features.h>

#define WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define WIFI_PASSWORD      CONFIG_ESP_WIFI_PASSWORD
#define MAX_RETRIES  CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED      BIT0
#define WIFI_FAILED         BIT1

static const char *TAG = "Measurement Node";

static int s_retry_num = 0;

esp_mqtt_client_handle_t mqtt_client;


void nvs_init(void){
    ESP_LOGI(TAG, "Initializing NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    ESP_LOGI(TAG, "WIFI event handler called");
    if (event_base == WIFI_EVENT){
        switch(event_id){
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                if (s_retry_num < MAX_RETRIES) {
                    esp_wifi_connect();
                    s_retry_num++;
                    ESP_LOGW(TAG, "retry to connect to the AP, retries: %d", s_retry_num);
                } else {
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAILED);
                }
                ESP_LOGE(TAG,"connect to the AP fail");
                break;
        }
    } 
    else if (event_base == IP_EVENT){
        switch(event_id){
            case IP_EVENT_STA_GOT_IP:
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
                s_retry_num = 0;
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED);
                break;
        }
    }
    else{
        ESP_LOGW(TAG, "Unhandled WiFi event ID: %ld", event_id);
    }
}

static void mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    ESP_LOGI(TAG, "MQTT event handler called");
    switch(event_id){
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected!");
            break;
        default:
            ESP_LOGW(TAG, "Unhandled MQTT Event ID: %ld", event_id);
            break;
    }
}

void wifi_init(void){
    ESP_LOGI(TAG, "Initializing WiFi");
    s_wifi_event_group = xEventGroupCreate();

    ESP_LOGI(TAG, "Initializing TCP/IP stack");
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_LOGI(TAG, "Initializing ESP event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    ESP_LOGI(TAG, "Defining Wifi configs");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold = {
                .authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD
            },
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_LOGI(TAG, "Creating event handler instances");
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    ESP_LOGI(TAG, "Setting configurations");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_LOGI(TAG, "Finished initializing WiFi");
}

void wifi_start(void){
    ESP_LOGI(TAG, "Starting WiFi");
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wifi initialization finished.");

    ESP_LOGI(TAG, "Waiting for event bits");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED | WIFI_FAILED, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", WIFI_SSID, WIFI_PASSWORD);
    } else if (bits & WIFI_FAILED) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", WIFI_SSID, WIFI_PASSWORD);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    ESP_LOGI(TAG, "Successfully connected to WiFi");
}


void mqtt_init(void){	
    ESP_LOGI(TAG, "Configuring MQTT client");
	const esp_mqtt_client_config_t mqtt_config = {
		.broker = {
			.address = {
				.uri = "mqtt://10.42.0.1:4000"
			}
		}
	};

	ESP_LOGI(TAG, "Initializing MQTT client");
	mqtt_client = esp_mqtt_client_init(&mqtt_config);

	ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_PUBLISHED, mqtt_event_handler, mqtt_client));

}

void mqtt_start(void){
    ESP_LOGI(TAG, "Starting MQTT client");
	ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));


}

extern "C" void app_main(void){
    nvs_init();

    wifi_init();

    wifi_start();

    mqtt_init();

    mqtt_start();

}