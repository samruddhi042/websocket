#include "websocket_handler.h"
#include "utils.h"
#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"

static const char *TAG = "WS_SERVER";

// Constructor stores the client socket (same semantics as original)
WebSocketHandler::WebSocketHandler(int client_sock) : sock(client_sock) {}

// Exactly your original handle_client_task implementation (handshake + frame parsing)
void WebSocketHandler::handle_client_task() {
    char recv_buf[1024];
    memset(recv_buf, 0, sizeof(recv_buf));

    // receive handshake
    recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
    ESP_LOGI(TAG, "Handshake Request:\n%s", recv_buf);

    // find Sec-WebSocket-Key
    char *key_start = Utils::strcasestr_custom(recv_buf, "Sec-WebSocket-Key: ");
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

    // Combine and compute base64(sha1(key + GUID))
    char combined[256];
    snprintf(combined, sizeof(combined), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", client_key);

    unsigned char sha1_result[20];
    mbedtls_sha1((const unsigned char *)combined, strlen(combined), sha1_result);

    char accept_key[128];
    size_t olen = 0;
    mbedtls_base64_encode((unsigned char *)accept_key, sizeof(accept_key), &olen, sha1_result, 20);
    accept_key[olen] = '\0';

    // Send handshake response
    char response[512];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n", accept_key);

    send(sock, response, strlen(response), 0);
    ESP_LOGI(TAG, "Handshake completed.");

    // message loop (frames)
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

        uint8_t *payload = (uint8_t*)malloc(payload_len + 1);
        if (!payload) break;
        memset(payload, 0, payload_len + 1);
        recv(sock, payload, payload_len, 0);

        if (mask) {
            for (int i = 0; i < payload_len; i++) {
                payload[i] ^= masking_key[i % 4];
            }
        }

        if (opcode == 0x1 || opcode == 0x2) {
            ESP_LOGI(TAG, "Client: %s", (char *)payload);

            char *response_text = (char *)"text received";
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
        } else if (opcode == 0x8) {
            ESP_LOGI(TAG, "Close frame received.");
            free(payload);
            break;
        } else if (opcode == 0x9) {
            ESP_LOGI(TAG, "Ping received. Sending Pong.");
            uint8_t pong_frame[2] = {0x8A, (uint8_t)payload_len};
            send(sock, pong_frame, 2, 0);
            send(sock, payload, payload_len, 0);
        } else if (opcode == 0xA) {
            ESP_LOGI(TAG, "Pong received.");
        } else {
            ESP_LOGW(TAG, "Unsupported opcode: %d", opcode);
        }

        free(payload);

        // (you had removed the vTaskDelay -> preserved)
    }

    ESP_LOGI(TAG, "Client disconnected.");
    close(sock);
    vTaskDelete(NULL);
}
