#include "wifi.c"
#include "mqtt.c"

void app_main(void){
    nvs_init();

    wifi_init();

    wifi_start();

    mqtt_init();

    mqtt_start();

    while(true){
        ESP_LOGI(TAG, "starting measurement loop");
        srand(time(NULL));
        char* measurement[sizeof(int)];
        sprintf(measurement, "%d", rand());
        mqtt_send_message("example measurement", measurement);
        vTaskDelay(CONFIG_MEASUREMENT_SEND_PERIOD);
    }

}