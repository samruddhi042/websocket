#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"

#define WIFI_SSID      "Doraemon"
#define WIFI_PASS      "gargi04**2006"

static const char *TAG = "WIFI_CONNECT";

void wifi_init_sta(void) {
    // Initialize the underlying TCP/IP stack
    esp_netif_init();
    esp_event_loop_create_default();

    // Create a default Wi-Fi station
    esp_netif_create_default_wifi_sta();

    // Initialize the Wi-Fi driver
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Set the Wi-Fi mode to station mode (STA)
    esp_wifi_set_mode(WIFI_MODE_STA);

    // Configure the Wi-Fi connection parameters (SSID and password)
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);

    // Start the Wi-Fi connection
    esp_wifi_start();
    ESP_LOGI(TAG, "Wi-Fi connecting...");

    // Attempt to connect
    esp_wifi_connect();
}

void app_main(void) {
    // Initialize the NVS flash storage
    nvs_flash_init();

    // Start Wi-Fi connection as a station
    wifi_init_sta();
}