#ifndef WEBSOCKET_HANDLER_H
#define WEBSOCKET_HANDLER_H

class WebSocketHandler {
public:
    explicit WebSocketHandler(int client_sock);
    // This drives the full client lifecycle (handshake + message loop)
    void handle_client_task();

private:
    int sock;
};

#endif // WEBSOCKET_HANDLER_H

