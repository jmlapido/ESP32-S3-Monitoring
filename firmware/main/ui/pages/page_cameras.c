#include "page_cameras.h"
#include "data_store.h"
#include "ui_theme.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "page_cam";

#define THUMB_W 114
#define THUMB_H 85
#define CAM_COLS 2
#define CAM_ROWS 2

static lv_obj_t *s_cam_panels[MAX_CAMERAS];
static lv_obj_t *s_cam_imgs[MAX_CAMERAS];
static lv_obj_t *s_cam_labels[MAX_CAMERAS];
static lv_obj_t *s_cam_status[MAX_CAMERAS];
static lv_obj_t *s_empty_label = NULL;
static lv_image_dsc_t s_img_dsc[MAX_CAMERAS];

void page_cameras_create(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Cameras");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(title, MD_COLOR_TEXT_SEC, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 8, 4);

    /* 2x2 grid container */
    lv_obj_t *grid = lv_obj_create(parent);
    lv_obj_set_size(grid, 240, 270);
    lv_obj_align(grid, LV_ALIGN_TOP_LEFT, 0, 20);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 3, 0);
    lv_obj_set_style_layout(grid, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP, 0);
    lv_obj_set_style_flex_main_place(grid, LV_FLEX_ALIGN_SPACE_EVENLY, 0);
    lv_obj_set_style_pad_row(grid, 4, 0);
    lv_obj_set_style_pad_column(grid, 4, 0);

    for (int i = 0; i < MAX_CAMERAS; i++) {
        s_cam_panels[i] = lv_obj_create(grid);
        lv_obj_set_size(s_cam_panels[i], THUMB_W, THUMB_H + 16);
        lv_obj_set_style_bg_color(s_cam_panels[i], MD_COLOR_SURFACE, 0);
        lv_obj_set_style_bg_opa(s_cam_panels[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(s_cam_panels[i], 4, 0);
        lv_obj_set_style_border_color(s_cam_panels[i], MD_COLOR_BORDER, 0);
        lv_obj_set_style_border_width(s_cam_panels[i], 1, 0);
        lv_obj_set_style_pad_all(s_cam_panels[i], 2, 0);
        lv_obj_clear_flag(s_cam_panels[i], LV_OBJ_FLAG_SCROLLABLE);

        /* Image placeholder */
        s_cam_imgs[i] = lv_image_create(s_cam_panels[i]);
        lv_obj_set_size(s_cam_imgs[i], THUMB_W - 4, THUMB_H - 4);
        lv_obj_align(s_cam_imgs[i], LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(s_cam_imgs[i], MD_COLOR_ELEVATED, 0);
        lv_obj_set_style_bg_opa(s_cam_imgs[i], LV_OPA_COVER, 0);

        /* Status dot */
        s_cam_status[i] = lv_obj_create(s_cam_panels[i]);
        lv_obj_set_size(s_cam_status[i], 6, 6);
        lv_obj_set_style_radius(s_cam_status[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(s_cam_status[i], MD_COLOR_TEXT_SEC, 0);
        lv_obj_set_style_bg_opa(s_cam_status[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_cam_status[i], 0, 0);
        lv_obj_align(s_cam_status[i], LV_ALIGN_BOTTOM_LEFT, 2, 0);

        /* Name label */
        s_cam_labels[i] = lv_label_create(s_cam_panels[i]);
        lv_label_set_text(s_cam_labels[i], "");
        lv_obj_set_style_text_font(s_cam_labels[i], &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(s_cam_labels[i], MD_COLOR_TEXT_SEC, 0);
        lv_label_set_long_mode(s_cam_labels[i], LV_LABEL_LONG_CLIP);
        lv_obj_set_width(s_cam_labels[i], THUMB_W - 20);
        lv_obj_align(s_cam_labels[i], LV_ALIGN_BOTTOM_LEFT, 12, 0);

        lv_obj_add_flag(s_cam_panels[i], LV_OBJ_FLAG_HIDDEN);

        /* Init image descriptor */
        memset(&s_img_dsc[i], 0, sizeof(lv_image_dsc_t));
    }

    s_empty_label = lv_label_create(grid);
    lv_label_set_text(s_empty_label, "No cameras configured.\nAdd RTSP monitors in web panel.");
    lv_obj_set_style_text_font(s_empty_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_empty_label, MD_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_text_align(s_empty_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_empty_label, 220);
    lv_obj_set_style_pad_top(s_empty_label, 40, 0);
}

void page_cameras_refresh(void)
{
    data_store_lock();
    const mondash_data_t *d = data_store_get();

    if (d->camera_count == 0) {
        for (int i = 0; i < MAX_CAMERAS; i++) {
            lv_obj_add_flag(s_cam_panels[i], LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_clear_flag(s_empty_label, LV_OBJ_FLAG_HIDDEN);
        data_store_unlock();
        return;
    }

    lv_obj_add_flag(s_empty_label, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < MAX_CAMERAS; i++) {
        if (i < d->camera_count) {
            lv_obj_clear_flag(s_cam_panels[i], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(s_cam_labels[i], d->cameras[i].name);
            lv_obj_set_style_bg_color(s_cam_status[i],
                d->cameras[i].is_online ? MD_COLOR_SUCCESS : MD_COLOR_ERROR, 0);

            /* Update thumbnail if available */
            if (d->cameras[i].thumb_rgb565 && d->cameras[i].thumb_w > 0) {
                s_img_dsc[i].header.w = d->cameras[i].thumb_w;
                s_img_dsc[i].header.h = d->cameras[i].thumb_h;
                s_img_dsc[i].header.cf = LV_COLOR_FORMAT_RGB565;
                s_img_dsc[i].data_size = d->cameras[i].thumb_w * d->cameras[i].thumb_h * 2;
                s_img_dsc[i].data = d->cameras[i].thumb_rgb565;
                lv_image_set_src(s_cam_imgs[i], &s_img_dsc[i]);
            }
        } else {
            lv_obj_add_flag(s_cam_panels[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    data_store_unlock();
}
