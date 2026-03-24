#pragma once

#include "lvgl.h"

/**
 * Initialize the ILI9341V SPI display and register with LVGL.
 * Allocates two 240x40 draw buffers in PSRAM.
 */
void display_hal_init(void);

/** Get the LVGL display handle. */
lv_display_t *display_hal_get_display(void);

/** Turn the backlight on (GPIO45 HIGH). */
void display_hal_backlight_on(void);

/** Turn the backlight off (GPIO45 LOW). */
void display_hal_backlight_off(void);
