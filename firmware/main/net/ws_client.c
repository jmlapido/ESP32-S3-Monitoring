#include "ws_client.h"
#include "json_parser.h"
#include "esp_websocket_client.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ws_client";

static esp_websocket_client_handle_t s_client = NULL;
static bool s_connected = false;
static int s_backoff_ms = 1000;
#define MAX_BACKOFF_MS 30000

static void ws_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    esp_websocket_event_data_t *ws_data = (esp_websocket_event_data_t *)data;

    switch (id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WebSocket connected to server");
        s_connected = true;
        s_backoff_ms = 1000;
        /* Send hello/identify */
        const char *hello = "{\"type\":\"hello\",\"device\":\"esp32\"}";
        esp_websocket_client_send_text(s_client, hello, strlen(hello), portMAX_DELAY);
        break;

    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "WebSocket disconnected");
        s_connected = false;
        break;

    case WEBSOCKET_EVENT_DATA:
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
        s_connected = false;
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
        .reconnect_timeout_ms = 5000,
        .network_timeout_ms = 10000,
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
    return s_connected;
}

void ws_client_send(const char *data, int len)
{
    if (s_client && s_connected) {
        esp_websocket_client_send_text(s_client, data, len, pdMS_TO_TICKS(1000));
    }
}
