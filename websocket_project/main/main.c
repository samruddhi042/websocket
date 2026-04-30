#include "wifi.h"
#include "ws_server.h"

void app_main(void) {
    wifi_init_sta();  // Connect to WiFi
    ws_server_start(); // Start WebSocket server
}
