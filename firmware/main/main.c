#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"

#include "display_hal.h"
#include "touch_hal.h"
#include "led_hal.h"
#include "wifi_manager.h"
#include "ws_client.h"
#include "data_store.h"
#include "ui_theme.h"
#include "ui_manager.h"

static const char *TAG = "mondash";

#define LVGL_TICK_PERIOD_MS 2
#define LVGL_TASK_STACK     (8 * 1024)
#define DATA_TASK_STACK     (4 * 1024)

static SemaphoreHandle_t s_lvgl_mutex = NULL;

/* LVGL tick callback for esp_timer */
static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

/* Data update callback from data_store — called from WS task context */
static void on_data_update(const char *page)
{
    /* Post refresh to LVGL task context via a flag or direct call
       since we protect LVGL with a mutex */
    if (xSemaphoreTake(s_lvgl_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        ui_manager_refresh_data(page);
        ui_manager_set_connection_status(ws_client_is_connected());
        xSemaphoreGive(s_lvgl_mutex);
    }
}

/* LVGL task — runs on core 0 */
static void lvgl_task(void *arg)
{
    ESP_LOGI(TAG, "LVGL task started on core %d", xPortGetCoreID());

    while (1) {
        if (xSemaphoreTake(s_lvgl_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            uint32_t next_ms = lv_timer_handler();
            xSemaphoreGive(s_lvgl_mutex);

            if (next_ms < 5) next_ms = 5;
            if (next_ms > 33) next_ms = 33;  /* cap at ~30fps */
            vTaskDelay(pdMS_TO_TICKS(next_ms));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

/* Status monitoring task — periodic updates */
static void status_task(void *arg)
{
    ESP_LOGI(TAG, "Status task started on core %d", xPortGetCoreID());
    int tick = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        tick++;

        /* Update connection status on UI every second */
        if (xSemaphoreTake(s_lvgl_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            ui_manager_set_connection_status(ws_client_is_connected());

            /* Update LED */
            if (ws_client_is_connected()) {
                led_hal_status_connected();
            } else if (wifi_manager_is_connected()) {
                led_hal_status_connecting();
            } else {
                led_hal_status_disconnected();
            }

            /* Simple time display (uptime-based since no NTP yet) */
            char time_buf[16];
            int mins = tick / 60;
            int secs = tick % 60;
            snprintf(time_buf, sizeof(time_buf), "%02d:%02d", mins, secs);
            ui_manager_update_time(time_buf);

            xSemaphoreGive(s_lvgl_mutex);
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "MonDash starting...");

    /* 1. LED — show blue (booting) */
    led_hal_init();
    led_hal_status_connecting();

    /* 2. LVGL init */
    lv_init();
    s_lvgl_mutex = xSemaphoreCreateMutex();
    assert(s_lvgl_mutex);

    /* 3. LVGL tick timer */
    const esp_timer_create_args_t tick_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    /* 4. Display & touch */
    display_hal_init();
    touch_hal_init();

    /* 5. UI theme and layout */
    ui_theme_init();
    ui_manager_init();

    /* 6. Data store */
    data_store_init();
    data_store_set_update_callback(on_data_update);

    /* 7. Start LVGL task on core 0 */
    xTaskCreatePinnedToCore(lvgl_task, "lvgl", LVGL_TASK_STACK, NULL, 5, NULL, 0);

    /* 8. WiFi */
    ESP_LOGI(TAG, "Connecting to WiFi...");
    wifi_manager_init();
    wifi_manager_wait_connected();

    /* 9. WebSocket */
    ESP_LOGI(TAG, "Starting WebSocket client...");
    ws_client_init();

    /* 10. Status monitoring task on core 1 */
    xTaskCreatePinnedToCore(status_task, "status", DATA_TASK_STACK, NULL, 3, NULL, 1);

    ESP_LOGI(TAG, "MonDash initialized successfully");
}
