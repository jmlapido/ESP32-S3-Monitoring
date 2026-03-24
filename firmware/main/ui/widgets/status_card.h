#pragma once

#include "lvgl.h"

typedef struct {
    lv_obj_t *card;
    lv_obj_t *lbl_title;
    lv_obj_t *lbl_value;
    lv_obj_t *status_dot;
} status_card_t;

status_card_t *status_card_create(lv_obj_t *parent, const char *title);
void status_card_set_value(status_card_t *card, const char *value);
void status_card_set_status(status_card_t *card, lv_color_t color);
