#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

class WebSocketServer {
public:
    // Start listening and accept clients; it spawns a task per client (same behavior)
    void start();
};

#endif // WEBSOCKET_SERVER_H
