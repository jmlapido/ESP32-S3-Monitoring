#include "ws_client.h"
#include "json_parser.h"
#include "esp_websocket_client.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ws_client";

static esp_websocket_client_handle_t s_client = NULL;
static volatile int64_t s_last_data_us = 0;  /* time of last received data (esp_timer_get_time) */
static int s_disconnected_ticks = 0;  /* watchdog counter */

static void ws_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    esp_websocket_event_data_t *ws_data = (esp_websocket_event_data_t *)data;

    switch (id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WebSocket connected to server");
        s_last_data_us = esp_timer_get_time();
        s_disconnected_ticks = 0;
        /* Send hello/identify */
        const char *hello = "{\"type\":\"hello\",\"device\":\"esp32\"}";
        esp_websocket_client_send_text(s_client, hello, strlen(hello), portMAX_DELAY);
        break;

    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "WebSocket disconnected");
        s_last_data_us = 0;
        s_disconnected_ticks = 0;  /* start watchdog counter */
        break;

    case WEBSOCKET_EVENT_DATA:
        s_last_data_us = esp_timer_get_time();  /* any data = alive */
        if (ws_data->op_code == 0x01 && ws_data->data_len > 0) {
            /* Text frame — parse JSON */
            char *buf = malloc(ws_data->data_len + 1);
            if (buf) {
                memcpy(buf, ws_data->data_ptr, ws_data->data_len);
                buf[ws_data->data_len] = '\0';
                json_parser_process(buf);
                free(buf);
            }
        }
        break;

    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "WebSocket error");
        /* Don't clear s_last_data_us — let the 60s timeout decide */
        break;

    default:
        break;
    }
}

void ws_client_init(void)
{
    ESP_LOGI(TAG, "Initializing WebSocket client -> %s", CONFIG_MONDASH_SERVER_URI);

    esp_websocket_client_config_t ws_cfg = {
        .uri = CONFIG_MONDASH_SERVER_URI,
        .buffer_size = 8192,
        .reconnect_timeout_ms = 10000,
        .network_timeout_ms = 30000,   /* high latency LXC/Tailscale routing */
        .ping_interval_sec = 30,       /* keep-alive: beat NAT idle timeout (~3.5 min) */
        .pingpong_timeout_sec = 120,   /* don't disconnect on slow pong — LXC latency */
        .task_stack = 8192,            /* default 4096 too small for JSON parsing */
    };

    s_client = esp_websocket_client_init(&ws_cfg);
    if (!s_client) {
        ESP_LOGE(TAG, "Failed to init WebSocket client");
        return;
    }

    esp_websocket_register_events(s_client, WEBSOCKET_EVENT_ANY, ws_event_handler, NULL);
    esp_websocket_client_start(s_client);

    ESP_LOGI(TAG, "WebSocket client started");
}

bool ws_client_is_connected(void)
{
    /* Connected if we received data within the last 60 seconds */
    if (!s_client || s_last_data_us == 0) return false;
    int64_t elapsed_us = esp_timer_get_time() - s_last_data_us;
    return elapsed_us < (60LL * 1000000LL);
}

void ws_client_send(const char *data, int len)
{
    if (s_client && ws_client_is_connected()) {
        esp_websocket_client_send_text(s_client, data, len, pdMS_TO_TICKS(1000));
    }
}

void ws_client_watchdog(void)
{
    if (!s_client) return;

    if (ws_client_is_connected()) {
        s_disconnected_ticks = 0;
        return;
    }

    s_disconnected_ticks++;
    /* If no data for 30+ seconds, force-restart the client */
    if (s_disconnected_ticks >= 30) {
        ESP_LOGW(TAG, "WS watchdog: forcing reconnect after %d ticks", s_disconnected_ticks);
        s_disconnected_ticks = 0;
        esp_websocket_client_stop(s_client);
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_websocket_client_start(s_client);
    }
}

void ws_client_send_heartbeat(void)
{
    if (s_client && ws_client_is_connected()) {
        const char *hb = "{\"type\":\"heartbeat\"}";
        esp_websocket_client_send_text(s_client, hb, strlen(hb), pdMS_TO_TICKS(500));
    }
}
