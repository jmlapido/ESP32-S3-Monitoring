#pragma once

#include "lvgl.h"

lv_obj_t *status_row_create(lv_obj_t *parent, const char *name, const char *value, lv_color_t dot_color);
void status_row_update(lv_obj_t *row, const char *name, const char *value, lv_color_t dot_color);
