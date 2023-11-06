#include "main.h"

esp_mqtt_client_handle_t mqtt_client;


static void mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    esp_mqtt_event_t *event = (esp_mqtt_event_t*)event_data;
    esp_mqtt_client_handle_t client =  event->client;
    ESP_LOGI(TAG, "MQTT event handler called");
    switch(event_id){
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected!");
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT message received");
            ESP_LOGI(TAG, "Topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Payload: %.*s", event->data_len, event->data);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT client disconnected by server!");
            break;
        default:
            ESP_LOGW(TAG, "Unhandled MQTT Event ID: %ld", event_id);
            break;
    }
}

void mqtt_init(void){	
    ESP_LOGI(TAG, "Configuring MQTT client");
	const esp_mqtt_client_config_t mqtt_config = {
		.broker = {
			.address = {
				.uri = CONFIG_MQTT_URI
			}
		}
	};

	ESP_LOGI(TAG, "Initializing MQTT client");
	mqtt_client = esp_mqtt_client_init(&mqtt_config);

	ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_PUBLISHED, mqtt_event_handler, mqtt_client));
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_DATA, mqtt_event_handler, mqtt_client));
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_DISCONNECTED, mqtt_event_handler, mqtt_client));

}

void mqtt_start(void){
    ESP_LOGI(TAG, "Starting MQTT client");
	ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));
	esp_mqtt_client_enqueue(mqtt_client, CONFIG_INITIAL_MESSAGE_TOPIC, CONFIG_INITIAL_MESSAGE_PAYLOAD, 0, 0, 0, true);
}

void mqtt_send_message(char* topic, char* payload){
    esp_mqtt_client_enqueue(mqtt_client, topic, payload, 0, 0, 0, true);
}