#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"

#define WIFI_SSID "Doremon"
#define WIFI_PASS "doraemon**2006"
#define SERVER_IP "192.168.1.100"  // Replace with your ESP32 server's IP
#define SERVER_PORT 8080

void tcp_client_task(void *pvParameters) {
    struct sockaddr_in server_addr;
    char *message = "Hello from ESP32 client";

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE("CLIENT", "Unable to create socket");
        vTaskDelete(NULL);
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        ESP_LOGE("CLIENT", "Unable to connect to server");
        close(sock);
        vTaskDelete(NULL);
    }

    send(sock, message, strlen(message), 0);
    ESP_LOGI("CLIENT", "Message sent");

    char rx_buffer[128];
    int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (len > 0) {
        rx_buffer[len] = 0;  // Null-terminate the string
        ESP_LOGI("CLIENT", "Received from server: %s", rx_buffer);
    }

    close(sock);
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("CLIENT", "Connecting to WiFi...");
    esp_wifi_connect();

    xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
}