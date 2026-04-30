**ESP32 WebSocket Server**

This project implements a fully custom WebSocket server on the ESP32 using ESP-IDF, without relying on any high-level WebSocket libraries. The server is built directly on top of raw TCP sockets (lwIP) and manually handles the complete WebSocket protocol flow, including HTTP upgrade handshake, SHA-1 hashing, Base64 encoding, and frame parsing.

The ESP32 connects to a Wi-Fi network in station mode, obtains an IP address, and listens for incoming WebSocket connections on port 8080. Once connected, it supports multiple clients simultaneously using FreeRTOS tasks, enabling real-time bidirectional communication between browsers, laptops, and mobile devices.

Incoming WebSocket frames are decoded manually, including payload length handling, masking/unmasking, and opcode interpretation. The server processes text messages and responds using a simple keyword-based conversational system, allowing it to behave like a lightweight chatbot running entirely on embedded hardware.

Unlike typical implementations that rely on libraries like ESPAsyncWebServer, this project demonstrates a low-level understanding of the WebSocket protocol and networking stack, making it useful for learning protocol internals, debugging, and building custom communication systems.

🔹Key Features 
Manual WebSocket handshake (HTTP Upgrade + SHA-1 + Base64)
Raw socket programming using lwIP (socket, bind, listen, accept)
Custom WebSocket frame parsing (opcode, payload length, masking)
Dynamic memory handling for payload processing
Bidirectional communication (client ↔ ESP32)
Multi-client support using FreeRTOS (xTaskCreate)
Keyword-based response system (basic conversational bot)
Ping/Pong and Close frame handling
Works with browser clients via JavaScript WebSocket API

-> At the end this is a low-level WebSocket server for ESP32 built using raw TCP sockets and ESP-IDF, implementing the full protocol manually and supporting real-time multi-client communication with custom frame handling.

**Project Context: **This project was developed as part of a Research & Development Cell task under TRF (The Robotics Forum) during FY Semester 2. Team Members: Samruddhi, Gargi
