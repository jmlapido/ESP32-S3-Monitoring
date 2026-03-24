#include "led_hal.h"
#include "led_strip.h"
#include "esp_log.h"

static const char *TAG = "led_hal";

#define LED_GPIO    GPIO_NUM_42
#define LED_COUNT   1

static led_strip_handle_t s_strip = NULL;

void led_hal_init(void)
{
    ESP_LOGI(TAG, "Initializing WS2812B LED on GPIO%d", LED_GPIO);

    led_strip_config_t strip_cfg = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_COUNT,
        .led_model = LED_MODEL_WS2812,  /* WS2812 implies GRB format */
    };

    led_strip_rmt_config_t rmt_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &s_strip));
    led_strip_clear(s_strip);

    ESP_LOGI(TAG, "LED initialized");
}

void led_hal_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    if (s_strip) {
        led_strip_set_pixel(s_strip, 0, r, g, b);
        led_strip_refresh(s_strip);
    }
}

void led_hal_off(void)
{
    if (s_strip) {
        led_strip_clear(s_strip);
    }
}

void led_hal_status_connected(void)
{
    led_hal_set_color(0, 30, 0);  /* dim green */
}

void led_hal_status_disconnected(void)
{
    led_hal_set_color(30, 0, 0);  /* dim red */
}

void led_hal_status_connecting(void)
{
    led_hal_set_color(0, 0, 30);  /* dim blue */
}

void led_hal_status_warning(void)
{
    led_hal_set_color(30, 20, 0);  /* dim amber */
}
