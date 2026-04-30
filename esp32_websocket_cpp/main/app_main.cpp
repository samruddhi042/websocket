#include "wifi_manager.h"
#include "websocket_server.h"

extern "C" void app_main() {
    WiFiManager wifi;
    wifi.init();

    WebSocketServer server;
    server.start();
}
