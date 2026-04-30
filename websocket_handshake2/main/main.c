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

#define MAX_CLIENTS 5
#define MAX_PAYLOAD_LEN 1024
#define PORT 8080

static const char *TAG = "WS_SERVER";

typedef struct {
    int sock;
    int active;
} client_t;

static client_t clients[MAX_CLIENTS];
static uint8_t payload_buf[MAX_PAYLOAD_LEN + 1]; // persistent buffer

char *strcasestr_custom(const char *haystack, const char *needle) {
    size_t needle_len = strlen(needle);
    for (; *haystack; ++haystack) {
        if (strncasecmp(haystack, needle, needle_len) == 0) {
            return (char *)haystack;
        }
    }
    return NULL;
}

static void websocket_handshake(int sock, char *request) {
    char *key_start = strcasestr_custom(request, "Sec-WebSocket-Key: ");
    if (!key_start) return;
    key_start += strlen("Sec-WebSocket-Key: ");
    char *key_end = strstr(key_start, "\r\n");
    if (!key_end) return;

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

    char response[256];
    int len = snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n", accept_key);

    send(sock, response, len, 0);
}

static void handle_websocket_frame(int sock) {
    uint8_t hdr[14];
    int hdr_len = recv(sock, hdr, 2, MSG_DONTWAIT);
    if (hdr_len <= 0) return;

    uint8_t opcode = hdr[0] & 0x0F;
    uint8_t mask = (hdr[1] >> 7) & 1;
    uint64_t payload_len = hdr[1] & 0x7F;

    if (payload_len == 126) {
        recv(sock, hdr + 2, 2, 0);
        payload_len = (hdr[2] << 8) | hdr[3];
    } else if (payload_len == 127) {
        // ignore very large frames
        return;
    }

    uint8_t masking_key[4];
    if (mask) recv(sock, masking_key, 4, 0);

    if (payload_len > MAX_PAYLOAD_LEN) return;

    recv(sock, payload_buf, payload_len, 0);
    payload_buf[payload_len] = '\0';

    if (mask) {
        for (int i = 0; i < payload_len; i++) {
            payload_buf[i] ^= masking_key[i % 4];
        }
    }

    if (opcode == 0x1) {
        // reply
        const char *msg = "text received";
        size_t msg_len = strlen(msg);
        uint8_t send_hdr[4];
        send_hdr[0] = 0x81; // FIN + text
        send_hdr[1] = msg_len;
        send(sock, send_hdr, 2, 0);
        send(sock, msg, msg_len, 0);
    }
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
    esp_wifi_connect();

    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    int yes = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY)
    };

    bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listen_sock, MAX_CLIENTS);

    fd_set read_fds;
    int max_fd = listen_sock;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(listen_sock, &read_fds);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active) {
                FD_SET(clients[i].sock, &read_fds);
                if (clients[i].sock > max_fd) max_fd = clients[i].sock;
            }
        }

        struct timeval tv = {.tv_sec = 0, .tv_usec = 5000};
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if (activity < 0) continue;

        if (FD_ISSET(listen_sock, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
            if (sock >= 0) {
                setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
                char handshake_buf[512];
                recv(sock, handshake_buf, sizeof(handshake_buf) - 1, 0);
                websocket_handshake(sock, handshake_buf);
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (!clients[i].active) {
                        clients[i].sock = sock;
                        clients[i].active = 1;
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && FD_ISSET(clients[i].sock, &read_fds)) {
                handle_websocket_frame(clients[i].sock);
            }
        }
    }
}
