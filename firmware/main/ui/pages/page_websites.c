#include "page_websites.h"
#include "status_row.h"
#include "data_store.h"
#include "ui_theme.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *s_list = NULL;
static lv_obj_t *s_rows[MAX_WEBSITES];
static lv_obj_t *s_empty_label = NULL;
static int s_row_count = 0;

void page_websites_create(lv_obj_t *parent)
{
    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Website Status");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(title, MD_COLOR_TEXT_SEC, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 8, 4);

    /* Scrollable list */
    s_list = lv_obj_create(parent);
    lv_obj_set_size(s_list, 240, 280);
    lv_obj_align(s_list, LV_ALIGN_TOP_LEFT, 0, 20);
    lv_obj_set_style_bg_opa(s_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_list, 0, 0);
    lv_obj_set_style_pad_all(s_list, 0, 0);
    lv_obj_set_style_layout(s_list, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(s_list, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_set_style_pad_row(s_list, 0, 0);

    /* Empty state */
    s_empty_label = lv_label_create(s_list);
    lv_label_set_text(s_empty_label, "No websites configured.\nAdd monitors in the web panel.");
    lv_obj_set_style_text_font(s_empty_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_empty_label, MD_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_text_align(s_empty_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_empty_label, 220);
    lv_obj_set_style_pad_top(s_empty_label, 40, 0);

    memset(s_rows, 0, sizeof(s_rows));
}

void page_websites_refresh(void)
{
    data_store_lock();
    const mondash_data_t *d = data_store_get();

    /* Remove old rows if count changed */
    if (s_row_count != d->website_count) {
        for (int i = 0; i < s_row_count; i++) {
            if (s_rows[i]) {
                lv_obj_delete(s_rows[i]);
                s_rows[i] = NULL;
            }
        }
        s_row_count = 0;
    }

    if (d->website_count == 0) {
        lv_obj_clear_flag(s_empty_label, LV_OBJ_FLAG_HIDDEN);
        data_store_unlock();
        return;
    }

    lv_obj_add_flag(s_empty_label, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < d->website_count; i++) {
        char val[16];
        snprintf(val, sizeof(val), "%d ms", d->websites[i].response_ms);
        lv_color_t color = d->websites[i].is_up ? MD_COLOR_SUCCESS : MD_COLOR_ERROR;

        if (s_rows[i]) {
            status_row_update(s_rows[i], d->websites[i].name, val, color);
        } else {
            s_rows[i] = status_row_create(s_list, d->websites[i].name, val, color);
        }
    }
    s_row_count = d->website_count;

    data_store_unlock();
}
