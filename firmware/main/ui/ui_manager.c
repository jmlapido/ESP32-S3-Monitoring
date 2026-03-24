#include "ui_manager.h"
#include "ui_theme.h"
#include "page_overview.h"
#include "page_websites.h"
#include "page_network.h"
#include "page_cameras.h"
#include "page_chart.h"
#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "ui_mgr";

#define TOP_BAR_H   16
#define SCREEN_W    240
#define SCREEN_H    320
#define CONTENT_H   (SCREEN_H - TOP_BAR_H)
#define NUM_PAGES   5
#define DOT_SIZE    6
#define DOT_GAP     10

static lv_obj_t *s_top_bar = NULL;
static lv_obj_t *s_lbl_title = NULL;
static lv_obj_t *s_lbl_time = NULL;
static lv_obj_t *s_conn_dot = NULL;
static lv_obj_t *s_tileview = NULL;
static lv_obj_t *s_dot_panel = NULL;
static lv_obj_t *s_dots[NUM_PAGES];
static lv_timer_t *s_dot_hide_timer = NULL;
static int s_current_page = 0;

static void dot_hide_cb(lv_timer_t *timer)
{
    lv_obj_add_flag(s_dot_panel, LV_OBJ_FLAG_HIDDEN);
}

static void show_dots(void)
{
    lv_obj_clear_flag(s_dot_panel, LV_OBJ_FLAG_HIDDEN);
    if (s_dot_hide_timer) {
        lv_timer_reset(s_dot_hide_timer);
    }
}

static void update_dots(int active)
{
    for (int i = 0; i < NUM_PAGES; i++) {
        if (i == active) {
            lv_obj_set_style_bg_color(s_dots[i], MD_COLOR_PRIMARY, 0);
            lv_obj_set_style_bg_opa(s_dots[i], LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_color(s_dots[i], MD_COLOR_TEXT_SEC, 0);
            lv_obj_set_style_bg_opa(s_dots[i], LV_OPA_50, 0);
        }
    }
}

static void tileview_cb(lv_event_t *e)
{
    lv_obj_t *tv = lv_event_get_target(e);
    lv_obj_t *tile = lv_tileview_get_tile_active(tv);

    int col = lv_obj_get_x(tile) / SCREEN_W;
    if (col != s_current_page) {
        s_current_page = col;
        update_dots(col);
        show_dots();
        ESP_LOGI(TAG, "Page changed to %d", col);
    }
}

static void create_top_bar(lv_obj_t *parent)
{
    s_top_bar = lv_obj_create(parent);
    lv_obj_set_size(s_top_bar, SCREEN_W, TOP_BAR_H);
    lv_obj_align(s_top_bar, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(s_top_bar, MD_COLOR_SURFACE, 0);
    lv_obj_set_style_bg_opa(s_top_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_top_bar, 0, 0);
    lv_obj_set_style_radius(s_top_bar, 0, 0);
    lv_obj_set_style_pad_all(s_top_bar, 2, 0);
    lv_obj_clear_flag(s_top_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    s_lbl_title = lv_label_create(s_top_bar);
    lv_label_set_text(s_lbl_title, "MonDash");
    lv_obj_set_style_text_font(s_lbl_title, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(s_lbl_title, MD_COLOR_TEXT, 0);
    lv_obj_align(s_lbl_title, LV_ALIGN_LEFT_MID, 2, 0);

    /* Connection dot */
    s_conn_dot = lv_obj_create(s_top_bar);
    lv_obj_set_size(s_conn_dot, 6, 6);
    lv_obj_set_style_radius(s_conn_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_conn_dot, MD_COLOR_ERROR, 0);
    lv_obj_set_style_bg_opa(s_conn_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_conn_dot, 0, 0);
    lv_obj_align(s_conn_dot, LV_ALIGN_LEFT_MID, 52, 0);

    /* Time */
    s_lbl_time = lv_label_create(s_top_bar);
    lv_label_set_text(s_lbl_time, "--:--");
    lv_obj_set_style_text_font(s_lbl_time, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(s_lbl_time, MD_COLOR_TEXT_SEC, 0);
    lv_obj_align(s_lbl_time, LV_ALIGN_RIGHT_MID, -2, 0);
}

static void create_dot_panel(lv_obj_t *parent)
{
    int total_w = NUM_PAGES * DOT_SIZE + (NUM_PAGES - 1) * DOT_GAP;

    s_dot_panel = lv_obj_create(parent);
    lv_obj_set_size(s_dot_panel, total_w + 16, DOT_SIZE + 10);
    lv_obj_align(s_dot_panel, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(s_dot_panel, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_dot_panel, LV_OPA_40, 0);
    lv_obj_set_style_radius(s_dot_panel, 8, 0);
    lv_obj_set_style_border_width(s_dot_panel, 0, 0);
    lv_obj_set_style_pad_all(s_dot_panel, 0, 0);
    lv_obj_clear_flag(s_dot_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_layout(s_dot_panel, LV_LAYOUT_FLEX, 0);
    lv_obj_set_style_flex_flow(s_dot_panel, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_style_flex_main_place(s_dot_panel, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(s_dot_panel, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_column(s_dot_panel, DOT_GAP, 0);

    for (int i = 0; i < NUM_PAGES; i++) {
        s_dots[i] = lv_obj_create(s_dot_panel);
        lv_obj_set_size(s_dots[i], DOT_SIZE, DOT_SIZE);
        lv_obj_set_style_radius(s_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(s_dots[i], 0, 0);
        lv_obj_clear_flag(s_dots[i], LV_OBJ_FLAG_SCROLLABLE);
    }
    update_dots(0);

    /* Auto-hide timer (2 seconds) */
    s_dot_hide_timer = lv_timer_create(dot_hide_cb, 2000, NULL);
    lv_timer_set_repeat_count(s_dot_hide_timer, -1);
}

void ui_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing UI manager");

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, MD_COLOR_BG, 0);

    /* Top bar */
    create_top_bar(scr);

    /* Tileview for swipeable pages */
    s_tileview = lv_tileview_create(scr);
    lv_obj_set_size(s_tileview, SCREEN_W, CONTENT_H);
    lv_obj_align(s_tileview, LV_ALIGN_TOP_LEFT, 0, TOP_BAR_H);
    lv_obj_set_style_bg_opa(s_tileview, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(s_tileview, tileview_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Create page tiles */
    lv_obj_t *tile0 = lv_tileview_add_tile(s_tileview, 0, 0, LV_DIR_RIGHT);
    lv_obj_t *tile1 = lv_tileview_add_tile(s_tileview, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_t *tile2 = lv_tileview_add_tile(s_tileview, 2, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_t *tile3 = lv_tileview_add_tile(s_tileview, 3, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_t *tile4 = lv_tileview_add_tile(s_tileview, 4, 0, LV_DIR_LEFT);

    /* Initialize pages */
    page_overview_create(tile0);
    page_websites_create(tile1);
    page_network_create(tile2);
    page_cameras_create(tile3);
    page_chart_create(tile4);

    /* Floating dot indicators */
    create_dot_panel(scr);

    ESP_LOGI(TAG, "UI initialized with %d pages", NUM_PAGES);
}

void ui_manager_update_time(const char *time_str)
{
    if (s_lbl_time) {
        lv_label_set_text(s_lbl_time, time_str);
    }
}

void ui_manager_set_connection_status(bool connected)
{
    if (s_conn_dot) {
        lv_obj_set_style_bg_color(s_conn_dot,
            connected ? MD_COLOR_SUCCESS : MD_COLOR_ERROR, 0);
    }
}

void ui_manager_refresh_data(const char *page)
{
    page_overview_refresh();

    if (strcmp(page, "websites") == 0) {
        page_websites_refresh();
    } else if (strcmp(page, "network") == 0) {
        page_network_refresh();
    } else if (strcmp(page, "cameras") == 0) {
        page_cameras_refresh();
    } else if (strcmp(page, "chart") == 0) {
        page_chart_refresh();
    }
}
