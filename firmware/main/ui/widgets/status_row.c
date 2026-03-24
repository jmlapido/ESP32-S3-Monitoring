#include "status_row.h"
#include "ui_theme.h"

#define ROW_H 28

lv_obj_t *status_row_create(lv_obj_t *parent, const char *name, const char *value, lv_color_t dot_color)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, lv_pct(100), ROW_H);
    lv_obj_set_style_bg_color(row, MD_COLOR_SURFACE, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(row, MD_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_pad_left(row, 8, 0);
    lv_obj_set_style_pad_right(row, 8, 0);
    lv_obj_set_style_pad_top(row, 4, 0);
    lv_obj_set_style_pad_bottom(row, 4, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    /* Status dot */
    lv_obj_t *dot = lv_obj_create(row);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, dot_color, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_align(dot, LV_ALIGN_LEFT_MID, 0, 0);

    /* Name label */
    lv_obj_t *lbl_name = lv_label_create(row);
    lv_label_set_text(lbl_name, name);
    lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_name, MD_COLOR_TEXT, 0);
    lv_label_set_long_mode(lbl_name, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lbl_name, 140);
    lv_obj_align(lbl_name, LV_ALIGN_LEFT_MID, 14, 0);

    /* Value label */
    lv_obj_t *lbl_val = lv_label_create(row);
    lv_label_set_text(lbl_val, value);
    lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_val, MD_COLOR_TEXT_SEC, 0);
    lv_obj_align(lbl_val, LV_ALIGN_RIGHT_MID, 0, 0);

    return row;
}

void status_row_update(lv_obj_t *row, const char *name, const char *value, lv_color_t dot_color)
{
    if (!row) return;

    /* Children: 0=dot, 1=name, 2=value */
    lv_obj_t *dot = lv_obj_get_child(row, 0);
    lv_obj_t *lbl_name = lv_obj_get_child(row, 1);
    lv_obj_t *lbl_val = lv_obj_get_child(row, 2);

    if (dot) lv_obj_set_style_bg_color(dot, dot_color, 0);
    if (lbl_name) lv_label_set_text(lbl_name, name);
    if (lbl_val) lv_label_set_text(lbl_val, value);
}
