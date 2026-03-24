#include "page_network.h"
#include "status_row.h"
#include "data_store.h"
#include "ui_theme.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *s_list = NULL;
static lv_obj_t *s_rows[MAX_DEVICES];
static lv_obj_t *s_empty_label = NULL;
static int s_row_count = 0;

void page_network_create(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Network Devices");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(title, MD_COLOR_TEXT_SEC, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 8, 4);

    s_list = lv_obj_create(parent);
    lv_obj_set_size(s_list, 240, 280);
    lv_obj_align(s_list, LV_ALIGN_TOP_LEFT, 0, 20);
    lv_obj_set_style_bg_opa(s_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_list, 0, 0);
    lv_obj_set_style_pad_all(s_list, 0, 0);
    lv_obj_set_style_layout(s_list, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(s_list, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_set_style_pad_row(s_list, 0, 0);

    s_empty_label = lv_label_create(s_list);
    lv_label_set_text(s_empty_label, "No network devices.\nAdd SNMP monitors in web panel.");
    lv_obj_set_style_text_font(s_empty_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_empty_label, MD_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_text_align(s_empty_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_empty_label, 220);
    lv_obj_set_style_pad_top(s_empty_label, 40, 0);

    memset(s_rows, 0, sizeof(s_rows));
}

void page_network_refresh(void)
{
    data_store_lock();
    const mondash_data_t *d = data_store_get();

    if (s_row_count != d->device_count) {
        for (int i = 0; i < s_row_count; i++) {
            if (s_rows[i]) {
                lv_obj_delete(s_rows[i]);
                s_rows[i] = NULL;
            }
        }
        s_row_count = 0;
    }

    if (d->device_count == 0) {
        lv_obj_clear_flag(s_empty_label, LV_OBJ_FLAG_HIDDEN);
        data_store_unlock();
        return;
    }

    lv_obj_add_flag(s_empty_label, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < d->device_count; i++) {
        char val[32];
        snprintf(val, sizeof(val), "%dc %.0fM",
            d->devices[i].client_count, d->devices[i].bandwidth_mbps);
        lv_color_t color = d->devices[i].is_online ? MD_COLOR_SUCCESS : MD_COLOR_ERROR;

        if (s_rows[i]) {
            status_row_update(s_rows[i], d->devices[i].name, val, color);
        } else {
            s_rows[i] = status_row_create(s_list, d->devices[i].name, val, color);
        }
    }
    s_row_count = d->device_count;

    data_store_unlock();
}
