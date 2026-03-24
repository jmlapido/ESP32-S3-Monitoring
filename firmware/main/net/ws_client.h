#pragma once

#include <stdbool.h>

void ws_client_init(void);
bool ws_client_is_connected(void);
void ws_client_send(const char *data, int len);
