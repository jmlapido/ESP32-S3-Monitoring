#pragma once

#include <stdint.h>
#include <stdbool.h>

#define MAX_WEBSITES    16
#define MAX_DEVICES     16
#define MAX_CAMERAS     4
#define MAX_CHART_POINTS 60
#define MAX_NAME_LEN    32

typedef struct {
    char name[MAX_NAME_LEN];
    bool is_up;
    int response_ms;
} website_status_t;

typedef struct {
    char name[MAX_NAME_LEN];
    bool is_online;
    int client_count;
    float bandwidth_mbps;
} device_status_t;

typedef struct {
    char name[MAX_NAME_LEN];
    bool is_online;
    uint8_t *thumb_rgb565;   /* 120x90 RGB565 in PSRAM, NULL if none */
    uint16_t thumb_w;
    uint16_t thumb_h;
} camera_status_t;

typedef struct {
    char title[MAX_NAME_LEN];
    float points[MAX_CHART_POINTS];
    int num_points;
} chart_data_t;

typedef struct {
    website_status_t websites[MAX_WEBSITES];
    int website_count;

    device_status_t devices[MAX_DEVICES];
    int device_count;

    camera_status_t cameras[MAX_CAMERAS];
    int camera_count;

    chart_data_t chart;

    /* Aggregate stats for overview */
    int websites_up;
    int devices_online;
    int cameras_online;
    int total_clients;
    float total_bandwidth;
    float avg_response_ms;
} mondash_data_t;

typedef void (*data_update_cb_t)(const char *page);

void data_store_init(void);
void data_store_lock(void);
void data_store_unlock(void);
const mondash_data_t *data_store_get(void);

/* Update functions (called from JSON parser, thread-safe) */
void data_store_update_websites(const website_status_t *sites, int count);
void data_store_update_devices(const device_status_t *devs, int count);
void data_store_update_cameras(const camera_status_t *cams, int count);
void data_store_update_chart(const char *title, const float *points, int count);

/* Register callback for UI updates (called on any data change) */
void data_store_set_update_callback(data_update_cb_t cb);
