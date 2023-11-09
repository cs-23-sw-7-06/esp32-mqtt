#include "main.h"

static int retries = 0;

static EventGroupHandle_t wifi_event_group;

#define WIFI_CONNECTED BIT0
#define WIFI_FAILED BIT1

void nvs_init() {
    ESP_LOGI(TAG, "Initializing NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "WIFI event handler called");
    if (event_base == WIFI_EVENT) {
        switch(event_id){
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                if (retries < MAX_RETRIES) {
                    esp_wifi_connect();
                    retries++;
                    ESP_LOGW(TAG, "retry to connect to the AP, retries: %d", retries);
                } else {
                    xEventGroupSetBits(wifi_event_group, WIFI_FAILED);
                }
                ESP_LOGE(TAG, "connect to the AP fail");
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
        case IP_EVENT_STA_GOT_IP:
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            retries = 0;
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED);
            break;
        }
    } else {
        ESP_LOGW(TAG, "Unhandled WiFi event ID: %ld", event_id);
    }
}

esp_err_t wifi_init(int argc, char **argv) {
    ESP_LOGI(TAG, "Initializing WiFi");
    wifi_event_group = xEventGroupCreate();

    ESP_LOGI(TAG, "Initializing TCP/IP stack");
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_LOGI(TAG, "Initializing ESP event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    ESP_LOGI(TAG, "Defining Wifi configs");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    wifi_config_t wifi_config = {
        .sta =
            {
                .ssid = WIFI_SSID,
                .password = WIFI_PASSWORD,
                .threshold = {.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD},
                .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
                .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
            },
    };

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_LOGI(TAG, "Creating event handler instances");
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    ESP_LOGI(TAG, "Setting configurations");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_LOGI(TAG, "Finished initializing WiFi");
    return ESP_OK;
}

esp_err_t wifi_start(int argc, char **argv) {
    ESP_LOGI(TAG, "Starting WiFi");
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wifi initialization finished.");

    ESP_LOGI(TAG, "Waiting for event bits");
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED | WIFI_FAILED, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", WIFI_SSID, WIFI_PASSWORD);
    } else if (bits & WIFI_FAILED) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", WIFI_SSID, WIFI_PASSWORD);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    ESP_LOGI(TAG, "Successfully connected to WiFi");
    return ESP_OK;
}
