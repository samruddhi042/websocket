idf_component_register(SRCS "main.c" "ws_server.c" "wifi.c"
                    INCLUDE_DIRS ".")
 idf_component_register(SRCS "main.c" "wifi.c" "ws_server.c"
INCLUDE_DIRS "."
REQUIRES mbedtls)