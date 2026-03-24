#include "json_parser.h"
#include "data_store.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <stdlib.h>

/* Base64 decode for camera thumbnails */
#include "mbedtls/base64.h"

static const char *TAG = "json_parser";

static void parse_websites(cJSON *data)
{
    cJSON *sites = cJSON_GetObjectItem(data, "sites");
    if (!cJSON_IsArray(sites)) return;

    int count = cJSON_GetArraySize(sites);
    if (count > MAX_WEBSITES) count = MAX_WEBSITES;

    website_status_t ws[MAX_WEBSITES];
    memset(ws, 0, sizeof(ws));

    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(sites, i);
        cJSON *name = cJSON_GetObjectItem(item, "name");
        cJSON *status = cJSON_GetObjectItem(item, "status");
        cJSON *ms = cJSON_GetObjectItem(item, "ms");

        if (cJSON_IsString(name)) {
            strncpy(ws[i].name, name->valuestring, MAX_NAME_LEN - 1);
        }
        ws[i].is_up = cJSON_IsString(status) && strcmp(status->valuestring, "up") == 0;
        ws[i].response_ms = cJSON_IsNumber(ms) ? (int)ms->valuedouble : 0;
    }

    data_store_update_websites(ws, count);
}

static void parse_network(cJSON *data)
{
    cJSON *devices = cJSON_GetObjectItem(data, "devices");
    if (!cJSON_IsArray(devices)) return;

    int count = cJSON_GetArraySize(devices);
    if (count > MAX_DEVICES) count = MAX_DEVICES;

    device_status_t devs[MAX_DEVICES];
    memset(devs, 0, sizeof(devs));

    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(devices, i);
        cJSON *name = cJSON_GetObjectItem(item, "name");
        cJSON *status = cJSON_GetObjectItem(item, "status");
        cJSON *clients = cJSON_GetObjectItem(item, "clients");
        cJSON *bw = cJSON_GetObjectItem(item, "bw_mbps");

        if (cJSON_IsString(name)) {
            strncpy(devs[i].name, name->valuestring, MAX_NAME_LEN - 1);
        }
        devs[i].is_online = cJSON_IsString(status) && strcmp(status->valuestring, "online") == 0;
        devs[i].client_count = cJSON_IsNumber(clients) ? (int)clients->valuedouble : 0;
        devs[i].bandwidth_mbps = cJSON_IsNumber(bw) ? (float)bw->valuedouble : 0;
    }

    data_store_update_devices(devs, count);
}

static void parse_cameras(cJSON *data)
{
    cJSON *cameras = cJSON_GetObjectItem(data, "cameras");
    if (!cJSON_IsArray(cameras)) return;

    int count = cJSON_GetArraySize(cameras);
    if (count > MAX_CAMERAS) count = MAX_CAMERAS;

    camera_status_t cams[MAX_CAMERAS];
    memset(cams, 0, sizeof(cams));

    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(cameras, i);
        cJSON *name = cJSON_GetObjectItem(item, "name");
        cJSON *status = cJSON_GetObjectItem(item, "status");
        cJSON *thumb = cJSON_GetObjectItem(item, "thumb_rgb565");
        cJSON *w = cJSON_GetObjectItem(item, "w");
        cJSON *h = cJSON_GetObjectItem(item, "h");

        if (cJSON_IsString(name)) {
            strncpy(cams[i].name, name->valuestring, MAX_NAME_LEN - 1);
        }
        cams[i].is_online = cJSON_IsString(status) && strcmp(status->valuestring, "online") == 0;
        cams[i].thumb_w = cJSON_IsNumber(w) ? (uint16_t)w->valuedouble : 0;
        cams[i].thumb_h = cJSON_IsNumber(h) ? (uint16_t)h->valuedouble : 0;

        /* Decode base64 thumbnail */
        if (cJSON_IsString(thumb) && cams[i].thumb_w > 0 && cams[i].thumb_h > 0) {
            size_t b64_len = strlen(thumb->valuestring);
            size_t out_len = cams[i].thumb_w * cams[i].thumb_h * 2;
            uint8_t *rgb = heap_caps_malloc(out_len, MALLOC_CAP_SPIRAM);
            if (rgb) {
                size_t decoded = 0;
                int ret = mbedtls_base64_decode(rgb, out_len, &decoded,
                    (const unsigned char *)thumb->valuestring, b64_len);
                if (ret == 0 && decoded == out_len) {
                    cams[i].thumb_rgb565 = rgb;
                } else {
                    ESP_LOGW(TAG, "Thumbnail decode failed for camera %d", i);
                    heap_caps_free(rgb);
                }
            }
        }
    }

    data_store_update_cameras(cams, count);

    /* Free temp thumbnail buffers (data_store copies them) */
    for (int i = 0; i < count; i++) {
        if (cams[i].thumb_rgb565) {
            heap_caps_free(cams[i].thumb_rgb565);
        }
    }
}

static void parse_chart(cJSON *data)
{
    cJSON *labels = cJSON_GetObjectItem(data, "labels");
    cJSON *series = cJSON_GetObjectItem(data, "series");
    if (!cJSON_IsArray(series)) return;

    int count = cJSON_GetArraySize(series);
    if (count > MAX_CHART_POINTS) count = MAX_CHART_POINTS;

    float points[MAX_CHART_POINTS];
    for (int i = 0; i < count; i++) {
        cJSON *val = cJSON_GetArrayItem(series, i);
        points[i] = cJSON_IsNumber(val) ? (float)val->valuedouble : 0;
    }

    data_store_update_chart("Bandwidth", points, count);
}

void json_parser_process(const char *json_str)
{
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        ESP_LOGW(TAG, "JSON parse error");
        return;
    }

    cJSON *type = cJSON_GetObjectItem(root, "type");
    cJSON *page = cJSON_GetObjectItem(root, "page");
    cJSON *data = cJSON_GetObjectItem(root, "data");

    if (!cJSON_IsString(type)) {
        cJSON_Delete(root);
        return;
    }

    const char *type_str = type->valuestring;

    if (strcmp(type_str, "update") == 0 && cJSON_IsString(page) && data) {
        const char *page_str = page->valuestring;
        if (strcmp(page_str, "websites") == 0) {
            parse_websites(data);
        } else if (strcmp(page_str, "network") == 0) {
            parse_network(data);
        } else if (strcmp(page_str, "cameras") == 0) {
            parse_cameras(data);
        }
    } else if (strcmp(type_str, "chart_data") == 0 && data) {
        parse_chart(data);
    } else if (strcmp(type_str, "config") == 0) {
        ESP_LOGI(TAG, "Received config update");
        /* Config handling can be extended later */
    }

    cJSON_Delete(root);
}
