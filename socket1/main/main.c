#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Include the server and client headers
void tcp_server_task(void *pvParameters);
void tcp_client_task(void *pvParameters);

void app_main(void) {
    int mode = 1;  // 1 for Server, 2 for Client

    if (mode == 1) {
        ESP_LOGI("MAIN", "Starting TCP Server");
        xTaskCreate(tcp_server_task, "tcp_server_task", 4096, NULL, 5, NULL);
    } else if (mode == 2) {
        ESP_LOGI("MAIN", "Starting TCP Client");
        xTaskCreate(tcp_client_task, "tcp_client_task", 4096, NULL, 5, NULL);
    }
}