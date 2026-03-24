#include "page_overview.h"
#include "status_card.h"
#include "data_store.h"
#include "ui_theme.h"
#include <stdio.h>

static status_card_t *s_card_websites = NULL;
static status_card_t *s_card_aps = NULL;
static status_card_t *s_card_cameras = NULL;
static status_card_t *s_card_latency = NULL;
static status_card_t *s_card_clients = NULL;
static status_card_t *s_card_bandwidth = NULL;

void page_overview_create(lv_obj_t *parent)
{
    lv_obj_set_style_pad_all(parent, 6, 0);
    lv_obj_set_style_layout(parent, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(parent, LV_FLEX_FLOW_ROW_WRAP, 0);
    lv_obj_set_style_flex_main_place(parent, LV_FLEX_ALIGN_SPACE_EVENLY, 0);
    lv_obj_set_style_pad_row(parent, 6, 0);
    lv_obj_set_style_pad_column(parent, 6, 0);

    s_card_websites  = status_card_create(parent, "Websites");
    s_card_aps       = status_card_create(parent, "APs");
    s_card_cameras   = status_card_create(parent, "Cameras");
    s_card_latency   = status_card_create(parent, "Avg Latency");
    s_card_clients   = status_card_create(parent, "Clients");
    s_card_bandwidth = status_card_create(parent, "Bandwidth");

    status_card_set_value(s_card_websites, "0/0");
    status_card_set_value(s_card_aps, "0/0");
    status_card_set_value(s_card_cameras, "0/0");
    status_card_set_value(s_card_latency, "-- ms");
    status_card_set_value(s_card_clients, "0");
    status_card_set_value(s_card_bandwidth, "0 Mbps");
}

void page_overview_refresh(void)
{
    data_store_lock();
    const mondash_data_t *d = data_store_get();
    char buf[32];

    /* Websites */
    snprintf(buf, sizeof(buf), "%d/%d", d->websites_up, d->website_count);
    status_card_set_value(s_card_websites, buf);
    status_card_set_status(s_card_websites,
        d->websites_up == d->website_count ? MD_COLOR_SUCCESS : MD_COLOR_ERROR);

    /* APs */
    snprintf(buf, sizeof(buf), "%d/%d", d->devices_online, d->device_count);
    status_card_set_value(s_card_aps, buf);
    status_card_set_status(s_card_aps,
        d->devices_online == d->device_count ? MD_COLOR_SUCCESS : MD_COLOR_ERROR);

    /* Cameras */
    snprintf(buf, sizeof(buf), "%d/%d", d->cameras_online, d->camera_count);
    status_card_set_value(s_card_cameras, buf);
    status_card_set_status(s_card_cameras,
        d->cameras_online == d->camera_count ? MD_COLOR_SUCCESS : MD_COLOR_ERROR);

    /* Latency */
    snprintf(buf, sizeof(buf), "%.0f ms", d->avg_response_ms);
    status_card_set_value(s_card_latency, buf);
    status_card_set_status(s_card_latency,
        d->avg_response_ms < 500 ? MD_COLOR_SUCCESS : MD_COLOR_WARNING);

    /* Clients */
    snprintf(buf, sizeof(buf), "%d", d->total_clients);
    status_card_set_value(s_card_clients, buf);
    status_card_set_status(s_card_clients, MD_COLOR_PRIMARY);

    /* Bandwidth */
    snprintf(buf, sizeof(buf), "%.0f Mbps", d->total_bandwidth);
    status_card_set_value(s_card_bandwidth, buf);
    status_card_set_status(s_card_bandwidth, MD_COLOR_PRIMARY);

    data_store_unlock();
}
