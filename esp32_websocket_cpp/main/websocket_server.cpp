#include "websocket_server.h"
#include "websocket_handler.h"
#include "config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <netinet/tcp.h> // TCP_NODELAY
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WS_SERVER";

void WebSocketServer::start() {
    struct sockaddr_in server_addr;
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Socket bind failed");
        close(listen_sock);
        return;
    }

    listen(listen_sock, 5);
    ESP_LOGI(TAG, "WebSocket server listening on port %d", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGW(TAG, "Socket accept failed");
            continue;
        }

        // Disable Nagle's algorithm for this client (same as your code)
        int flag = 1;
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

        ESP_LOGI(TAG, "Client connected. Creating handler task...");

        // Create handler instance and spawn a task that runs handle_client_task
        WebSocketHandler *handler = new WebSocketHandler(sock);
        xTaskCreate([](void *param) {
            WebSocketHandler *h = static_cast<WebSocketHandler*>(param);
            h->handle_client_task();
            delete h;
        }, "handle_client", 8192, handler, 5, NULL);
    }
}
