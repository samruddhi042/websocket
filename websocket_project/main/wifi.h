#pragma once

#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"

void wifi_init_sta(void);
