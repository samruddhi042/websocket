#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"
#include <ctype.h>
#include <netinet/tcp.h> // for TCP_NODELAY

#define MAX_PAYLOAD_LEN 1024
#define PORT 8080
static const char *TAG = "WS_SERVER";

static void handle_client_task(void *pvParameters);
char *strcasestr_custom(const char *haystack, const char *needle) {
    size_t needle_len = strlen(needle);
    for (; *haystack; ++haystack) {
        if (strncasecmp(haystack, needle, needle_len) == 0) {
            return (char *)haystack;
        }
    }
    return NULL;
}

void app_main() {
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
            .password = "gargi04**2006",
        },
    };

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();

    // ✅ Disable Wi-Fi power saving to reduce latency
    esp_wifi_set_ps(WIFI_PS_NONE);

    esp_wifi_connect();

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

     
        int flag = 1;
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

        ESP_LOGI(TAG, "Client connected. Creating handler task...");
        xTaskCreate(handle_client_task, "handle_client", 8192, (void *)(intptr_t)sock, 5, NULL);
    }
}

// Handle each WebSocket client
static void handle_client_task(void *pvParameters) {
    int sock = (intptr_t)pvParameters;
    char recv_buf[1024];
    memset(recv_buf, 0, sizeof(recv_buf));

    recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
    ESP_LOGI(TAG, "Handshake Request:\n%s", recv_buf);

    char *key_start = strcasestr_custom(recv_buf, "Sec-WebSocket-Key: ");
    if (!key_start) {
        ESP_LOGE(TAG, "WebSocket key not found.");
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    key_start += strlen("Sec-WebSocket-Key: ");
    char *key_end = strstr(key_start, "\r\n");
    if (!key_end) {
        ESP_LOGE(TAG, "Malformed key line.");
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    char client_key[128] = {0};
    strncpy(client_key, key_start, key_end - key_start);

    char combined[256];
    snprintf(combined, sizeof(combined), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", client_key);

    unsigned char sha1_result[20];
    mbedtls_sha1((const unsigned char *)combined, strlen(combined), sha1_result);

    char accept_key[128];
    size_t olen = 0;
    mbedtls_base64_encode((unsigned char *)accept_key, sizeof(accept_key), &olen, sha1_result, 20);
    accept_key[olen] = '\0';

    char response[512];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n", accept_key);

    send(sock, response, strlen(response), 0);
    ESP_LOGI(TAG, "Handshake completed.");

    while (1) {
        uint8_t hdr[14] = {0};
        int hdr_len = recv(sock, hdr, 2, 0);
        if (hdr_len <= 0) break;

        uint8_t opcode = hdr[0] & 0x0F;
        uint8_t mask = (hdr[1] >> 7) & 1;
        uint64_t payload_len = hdr[1] & 0x7F;

        if (payload_len == 126) {
            recv(sock, hdr + 2, 2, 0);
            payload_len = (hdr[2] << 8) | hdr[3];
        } else if (payload_len == 127) {
            recv(sock, hdr + 2, 8, 0);
            ESP_LOGW(TAG, "Payload too large");
            break;
        }

        uint8_t masking_key[4] = {0};
        if (mask) recv(sock, masking_key, 4, 0);

        if (payload_len > MAX_PAYLOAD_LEN) {
            ESP_LOGW(TAG, "Payload exceeded max");
            break;
        }

        uint8_t *payload = malloc(payload_len + 1);
        if (!payload) break;
        memset(payload, 0, payload_len + 1);
        recv(sock, payload, payload_len, 0);

        if (mask) {
            for (int i = 0; i < payload_len; i++) {
                payload[i] ^= masking_key[i % 4];
            }
        }

        if (opcode == 0x1 || opcode == 0x2) {
            ESP_LOGI(TAG, "%s", (char *)payload);

            char *response_text = "text received";
            size_t reply_len = strlen(response_text);
           uint8_t send_hdr[10] = {0};
            int send_len = 0;

            send_hdr[0] = 0x81; // FIN + Text Frame
            if (reply_len <= 125) {
               send_hdr[1] = reply_len;
               send_len = 2;
           } else if (reply_len <= 65535) {
              send_hdr[1] = 126;
               send_hdr[2] = (reply_len >> 8) & 0xFF;
               send_hdr[3] = reply_len & 0xFF;
               send_len = 4;
          }

           send(sock, send_hdr, send_len, 0);
           send(sock, response_text, reply_len, 0);
           
         } 
           else if (opcode == 0x8) {
           // ESP_LOGI(TAG, "Close frame received.");
            free(payload);
            break;
        } else if (opcode == 0x9) {
        //    ESP_LOGI(TAG, "Ping received. Sending Pong.");
            uint8_t pong_frame[2] = {0x8A, payload_len};
            send(sock, pong_frame, 2, 0);
            send(sock, payload, payload_len, 0);
        } else if (opcode == 0xA) {
            // ESP_LOGI(TAG, "Pong received.");
        } else {
        //    ESP_LOGW(TAG, "Unsupported opcode: %d", opcode);
        }

        free(payload);

        // ✅ Reduced artificial delay to 2ms
        //vTaskDelay(2 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Client disconnected.");
    close(sock);
    vTaskDelete(NULL);
}