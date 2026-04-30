#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"

#define WIFI_SSID "Doraemon"
#define WIFI_PASS "doraemon**2006"
#define SERVER_PORT 8080

void tcp_server_task(void *pvParameters) {
    char rx_buffer[128];
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        ESP_LOGE("SERVER", "Unable to create socket");
        vTaskDelete(NULL);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE("SERVER", "Unable to bind socket");
        vTaskDelete(NULL);
    }

    listen(server_socket, 1);
    ESP_LOGI("SERVER", "Waiting for client connection...");

    int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
    if (client_socket < 0) {
        ESP_LOGE("SERVER", "Unable to accept connection");
        vTaskDelete(NULL);
    }

    ESP_LOGI("SERVER", "Client connected");

    while (1) {
        int len = recv(client_socket, rx_buffer, sizeof(rx_buffer), 0);
        if (len > 0) {
            rx_buffer[len] = 0;  // Null-terminate the string
            ESP_LOGI("SERVER", "Received: %s", rx_buffer);
            send(client_socket, "Message received", 17, 0);
        }
    }

    close(client_socket);
    close(server_socket);
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("SERVER", "Connecting to WiFi...");
    esp_wifi_connect();

    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
}