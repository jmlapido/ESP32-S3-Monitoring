#include "data_store.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "data_store";

static mondash_data_t *s_data = NULL;
static SemaphoreHandle_t s_mutex = NULL;
static data_update_cb_t s_update_cb = NULL;

void data_store_init(void)
{
    ESP_LOGI(TAG, "Initializing data store in PSRAM");
    s_data = heap_caps_calloc(1, sizeof(mondash_data_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(s_data);
    s_mutex = xSemaphoreCreateMutex();
    assert(s_mutex);
}

void data_store_lock(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
}

void data_store_unlock(void)
{
    xSemaphoreGive(s_mutex);
}

const mondash_data_t *data_store_get(void)
{
    return s_data;
}

static void recalc_aggregates(void)
{
    int up = 0, online_dev = 0, online_cam = 0, clients = 0;
    float bw = 0, resp_total = 0;

    for (int i = 0; i < s_data->website_count; i++) {
        if (s_data->websites[i].is_up) up++;
        resp_total += s_data->websites[i].response_ms;
    }
    for (int i = 0; i < s_data->device_count; i++) {
        if (s_data->devices[i].is_online) online_dev++;
        clients += s_data->devices[i].client_count;
        bw += s_data->devices[i].bandwidth_mbps;
    }
    for (int i = 0; i < s_data->camera_count; i++) {
        if (s_data->cameras[i].is_online) online_cam++;
    }

    s_data->websites_up = up;
    s_data->devices_online = online_dev;
    s_data->cameras_online = online_cam;
    s_data->total_clients = clients;
    s_data->total_bandwidth = bw;
    s_data->avg_response_ms = s_data->website_count > 0
        ? resp_total / s_data->website_count : 0;
}

void data_store_update_websites(const website_status_t *sites, int count)
{
    data_store_lock();
    if (count > MAX_WEBSITES) count = MAX_WEBSITES;
    memcpy(s_data->websites, sites, count * sizeof(website_status_t));
    s_data->website_count = count;
    recalc_aggregates();
    data_store_unlock();

    if (s_update_cb) s_update_cb("websites");
}

void data_store_update_devices(const device_status_t *devs, int count)
{
    data_store_lock();
    if (count > MAX_DEVICES) count = MAX_DEVICES;
    memcpy(s_data->devices, devs, count * sizeof(device_status_t));
    s_data->device_count = count;
    recalc_aggregates();
    data_store_unlock();

    if (s_update_cb) s_update_cb("network");
}

void data_store_update_cameras(const camera_status_t *cams, int count)
{
    data_store_lock();
    if (count > MAX_CAMERAS) count = MAX_CAMERAS;
    /* Copy metadata but not thumbnail pointers — those are managed separately */
    for (int i = 0; i < count; i++) {
        strncpy(s_data->cameras[i].name, cams[i].name, MAX_NAME_LEN - 1);
        s_data->cameras[i].is_online = cams[i].is_online;
        s_data->cameras[i].thumb_w = cams[i].thumb_w;
        s_data->cameras[i].thumb_h = cams[i].thumb_h;
        if (cams[i].thumb_rgb565) {
            size_t sz = cams[i].thumb_w * cams[i].thumb_h * 2;
            if (!s_data->cameras[i].thumb_rgb565) {
                s_data->cameras[i].thumb_rgb565 = heap_caps_malloc(sz, MALLOC_CAP_SPIRAM);
            }
            if (s_data->cameras[i].thumb_rgb565) {
                memcpy(s_data->cameras[i].thumb_rgb565, cams[i].thumb_rgb565, sz);
            }
        }
    }
    s_data->camera_count = count;
    recalc_aggregates();
    data_store_unlock();

    if (s_update_cb) s_update_cb("cameras");
}

void data_store_update_chart(const char *title, const float *points, int count)
{
    data_store_lock();
    if (count > MAX_CHART_POINTS) count = MAX_CHART_POINTS;
    strncpy(s_data->chart.title, title, MAX_NAME_LEN - 1);
    memcpy(s_data->chart.points, points, count * sizeof(float));
    s_data->chart.num_points = count;
    data_store_unlock();

    if (s_update_cb) s_update_cb("chart");
}

void data_store_set_update_callback(data_update_cb_t cb)
{
    s_update_cb = cb;
}
