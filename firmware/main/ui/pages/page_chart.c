#include "page_chart.h"
#include "data_store.h"
#include "ui_theme.h"
#include <string.h>

static lv_obj_t *s_chart = NULL;
static lv_chart_series_t *s_series = NULL;
static lv_obj_t *s_title_label = NULL;

void page_chart_create(lv_obj_t *parent)
{
    s_title_label = lv_label_create(parent);
    lv_label_set_text(s_title_label, "Bandwidth");
    lv_obj_set_style_text_font(s_title_label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(s_title_label, MD_COLOR_TEXT_SEC, 0);
    lv_obj_align(s_title_label, LV_ALIGN_TOP_LEFT, 8, 4);

    /* Chart */
    s_chart = lv_chart_create(parent);
    lv_obj_set_size(s_chart, 224, 260);
    lv_obj_align(s_chart, LV_ALIGN_TOP_MID, 0, 22);

    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, MAX_CHART_POINTS);
    lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 200);
    lv_chart_set_div_line_count(s_chart, 5, 8);
    lv_chart_set_update_mode(s_chart, LV_CHART_UPDATE_MODE_SHIFT);

    /* Dark theme styling */
    lv_obj_set_style_bg_color(s_chart, MD_COLOR_SURFACE, 0);
    lv_obj_set_style_bg_opa(s_chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_chart, MD_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(s_chart, 1, 0);
    lv_obj_set_style_radius(s_chart, 6, 0);
    lv_obj_set_style_line_color(s_chart, MD_COLOR_BORDER, LV_PART_MAIN);
    lv_obj_set_style_line_width(s_chart, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_chart, 8, 0);

    /* Series */
    s_series = lv_chart_add_series(s_chart, MD_COLOR_PRIMARY, LV_CHART_AXIS_PRIMARY_Y);

    /* Style the series line */
    lv_obj_set_style_line_width(s_chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(s_chart, 0, 0, LV_PART_INDICATOR);  /* hide data points */

    /* Initialize with zeros */
    for (int i = 0; i < MAX_CHART_POINTS; i++) {
        lv_chart_set_next_value(s_chart, s_series, 0);
    }
}

void page_chart_refresh(void)
{
    data_store_lock();
    const mondash_data_t *d = data_store_get();

    if (d->chart.num_points > 0) {
        lv_label_set_text(s_title_label, d->chart.title);

        /* Find max value for auto-scaling */
        float max_val = 1;
        for (int i = 0; i < d->chart.num_points; i++) {
            if (d->chart.points[i] > max_val) max_val = d->chart.points[i];
        }
        int range_max = (int)(max_val * 1.2f);
        if (range_max < 10) range_max = 10;
        lv_chart_set_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 0, range_max);

        /* Set all points */
        for (int i = 0; i < d->chart.num_points; i++) {
            s_series->y_points[i] = (int32_t)d->chart.points[i];
        }
        /* Fill remaining with LV_CHART_POINT_NONE */
        for (int i = d->chart.num_points; i < MAX_CHART_POINTS; i++) {
            s_series->y_points[i] = LV_CHART_POINT_NONE;
        }
        lv_chart_refresh(s_chart);
    }

    data_store_unlock();
}
