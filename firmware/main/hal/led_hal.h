#pragma once

#include <stdint.h>

void led_hal_init(void);
void led_hal_set_color(uint8_t r, uint8_t g, uint8_t b);
void led_hal_off(void);

/* Convenience status colors */
void led_hal_status_connected(void);
void led_hal_status_disconnected(void);
void led_hal_status_connecting(void);
void led_hal_status_warning(void);
