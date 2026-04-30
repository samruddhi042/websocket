cmake_minimum_required(VERSION 3.16)

# Project name
set(PROJECT_NAME "ws_server")
project(${PROJECT_NAME})

# ESP-IDF component registration
idf_component_register(
    SRCS
        "main.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        esp_wifi
        esp_event
        nvs_flash
        lwip
        mbedtls
)