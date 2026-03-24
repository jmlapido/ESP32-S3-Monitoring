#pragma once

#include <stdbool.h>
#include <stdint.h>

void wifi_manager_init(void);
bool wifi_manager_is_connected(void);
int8_t wifi_manager_get_rssi(void);
void wifi_manager_wait_connected(void);
