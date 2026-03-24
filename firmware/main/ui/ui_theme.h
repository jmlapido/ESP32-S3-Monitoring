#pragma once

#include "lvgl.h"

/* MonDash dark color palette */
#define MD_COLOR_BG         lv_color_hex(0x0D1117)
#define MD_COLOR_SURFACE    lv_color_hex(0x161B22)
#define MD_COLOR_ELEVATED   lv_color_hex(0x1C2128)
#define MD_COLOR_BORDER     lv_color_hex(0x30363D)
#define MD_COLOR_PRIMARY    lv_color_hex(0x58A6FF)
#define MD_COLOR_SUCCESS    lv_color_hex(0x3FB950)
#define MD_COLOR_ERROR      lv_color_hex(0xF85149)
#define MD_COLOR_WARNING    lv_color_hex(0xD29922)
#define MD_COLOR_TEXT       lv_color_hex(0xE6EDF3)
#define MD_COLOR_TEXT_SEC   lv_color_hex(0x8B949E)

void ui_theme_init(void);
