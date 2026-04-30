#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#define PORT 3333  // TCP port
#define WIFI_SSID "Doraemon"
#define WIFI_PASS "gargi04**2006"

static const char *TAG = "TCP_SERVER";

// ========== Wi-Fi AP (Access Point) Setup ==========
void wifi_init_softap(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t ap_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen(WIFI_PASS) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "WiFi AP started. SSID:%s password:%s", WIFI_SSID, WIFI_PASS);
}

// ========== TCP Socket Server ==========
void tcp_server_task(void *pvParameters) {
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        listen(listen_sock, 1);

        ESP_LOGI(TAG, "Socket listening on port %d", PORT);

        struct sockaddr_in6 source_addr;
        uint addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            close(listen_sock);
            continue;
        }

        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        while (1) {
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            } else if (len == 0) {
                ESP_LOGI(TAG, "Connection closed");
                break;
            } else {
                rx_buffer[len] = 0; // Null-terminate
                ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

                // Echo back
                send(sock, rx_buffer, len, 0);
            }
        }

        close(sock);
        close(listen_sock);
        ESP_LOGI(TAG, "Socket closed, restarting server");
    }

    vTaskDelete(NULL);
}

// ========== Main App ==========
void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
        nvs_flash_erase(), nvs_flash_init();

    wifi_init_softap();
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
}