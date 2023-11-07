#include "wifi.c"
#include "mqtt.c"

void measurement_loop(void){
    ESP_LOGI(TAG, "starting measurement loop");
    while(true){
        srand(time(NULL));
        char* measurement[sizeof(int)];
        sprintf(measurement, "%d", rand());
        mqtt_send_message("example measurement", measurement);
        long now, then;
        now = then = clock();
        while(now-then < (CLOCKS_PER_SEC/1000)*CONFIG_MEASUREMENT_SEND_PERIOD) now = clock();
    }
}

static struct {
    struct arg_str *topic;
    struct arg_str *payload;
    struct arg_end *end;
} send_args;

int send(int argc, char** argv){
    arg_parse(argc, argv, (void**) &send_args);
    ESP_LOGI(TAG, "Topic: %s", send_args.topic->sval[0]);
    ESP_LOGI(TAG, "Payload: %s", send_args.payload->sval[0]);
    mqtt_send_message(send_args.topic->sval[0], send_args.payload->sval[0]);
    return 0;
}

int state = 0;

int state_machine(void){
    switch(state){
        case 0:
            nvs_init();
            break;
        case 1:
            wifi_init();
            break;
        case 2:
            wifi_start();
            break;
        case 3:
            mqtt_init();
            break;
        case 4:
            mqtt_start();
            break;
        case 5:
            measurement_loop();
            break;
        default:
            ESP_LOGW(TAG, "All states in state machine has been covered!");
            return 1;
    }
    state++;
    return 0;
}

int init_all(){
    wifi_init();    
    wifi_start();   
    mqtt_init();    
    mqtt_start();   
    state = 5;
    return 0;
}

void app_main(void){
    nvs_init(); 
    if(DEBUG_MODE){ 
        ESP_LOGI(TAG, "Entering debug mode");
        esp_console_repl_t *repl = NULL;
        esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
        repl_config.prompt =  "esp-debug >";
        repl_config.max_cmdline_length = 15;
        ESP_ERROR_CHECK(esp_console_register_help_command());
        const esp_console_cmd_t run_cmd = {
            .command = "run",
            .help = "step through the program",
            .hint = NULL,
            .func = &state_machine,
        };
        ESP_ERROR_CHECK(esp_console_cmd_register(&run_cmd));

        send_args.topic = arg_str1(NULL, NULL, "<topic>", "Definition of message topic");
        send_args.payload = arg_str1(NULL, NULL, "<payload>", "Definition of message payload");
        send_args.end = arg_end(2);
        const esp_console_cmd_t send_cmd = {
            .command = "send",
            .help = "send a message to the server node",
            .hint = NULL,
            .func = &send,
            .argtable = &send_args
        };
        ESP_ERROR_CHECK(esp_console_cmd_register(&send_cmd));

        const esp_console_cmd_t init_all_cmd = {
            .command = "init_all",
            .help = "Initialize and start everything",
            .hint = NULL,
            .func = &init_all
        };
        ESP_ERROR_CHECK(esp_console_cmd_register(&init_all_cmd));

        const esp_console_cmd_t wifi_init_cmd = {
            .command = "init_wifi",
            .help = "Initialize Wi-Fi",
            .hint = NULL,
            .func = &wifi_init
        };
        ESP_ERROR_CHECK(esp_console_cmd_register(&wifi_init_cmd));

        const esp_console_cmd_t wifi_start_cmd = {
            .command = "start_wifi",
            .help = "Start and connect wifi",
            .hint = NULL,
            .func = &wifi_start
        };
        ESP_ERROR_CHECK(esp_console_cmd_register(&wifi_start_cmd));

        const esp_console_cmd_t mqtt_init_cmd = {
            .command = "init_mqtt",
            .help = "Initialize MQTT",
            .hint = NULL,
            .func = &mqtt_init
        };
        ESP_ERROR_CHECK(esp_console_cmd_register(&mqtt_init_cmd));

        const esp_console_cmd_t mqtt_start_cmd = {
            .command = "start_mqtt",
            .help = "Start and connect MQTT",
            .hint = NULL,
            .func = &mqtt_start
        };
        ESP_ERROR_CHECK(esp_console_cmd_register(&mqtt_start_cmd));

        const esp_console_cmd_t measurement_cmd = {
            .command = "start_measurement",
            .help = "Start measuring",
            .hint = NULL,
            .func = &measurement_loop
        };
        ESP_ERROR_CHECK(esp_console_cmd_register(&measurement_cmd));

        esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
        ESP_ERROR_CHECK(esp_console_start_repl(repl));
    }
    else{
        init_all();
        measurement_loop();
    }
}