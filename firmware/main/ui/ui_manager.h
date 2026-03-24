#pragma once

#include <stdbool.h>

void ui_manager_init(void);
void ui_manager_update_time(const char *time_str);
void ui_manager_set_connection_status(bool connected);
void ui_manager_refresh_data(const char *page);
