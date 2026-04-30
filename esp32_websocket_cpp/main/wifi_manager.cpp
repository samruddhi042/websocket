#include "wifi_manager.h"
#include "config.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_netif.h"

static const char *TAG = "WS_SERVER";

// Initialize NVS, network stack and start Wi-Fi in STA mode with the same credentials
void WiFiManager::init() {
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "Doraemon",
            .password = "doraemon04**2006",
        },
    };

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();

    // Disable Wi-Fi power saving to reduce latency (same as your update)
    esp_wifi_set_ps(WIFI_PS_NONE);

    esp_wifi_connect();

    ESP_LOGI(TAG, "WiFi init complete and connect called.");
}
