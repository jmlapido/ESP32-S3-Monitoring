#include "status_card.h"
#include "ui_theme.h"
#include "esp_heap_caps.h"

status_card_t *status_card_create(lv_obj_t *parent, const char *title)
{
    status_card_t *sc = heap_caps_calloc(1, sizeof(status_card_t), MALLOC_CAP_SPIRAM);
    if (!sc) return NULL;

    sc->card = lv_obj_create(parent);
    lv_obj_set_size(sc->card, 110, 45);
    lv_obj_set_style_bg_color(sc->card, MD_COLOR_SURFACE, 0);
    lv_obj_set_style_bg_opa(sc->card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(sc->card, 6, 0);
    lv_obj_set_style_border_color(sc->card, MD_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(sc->card, 1, 0);
    lv_obj_set_style_pad_all(sc->card, 6, 0);
    lv_obj_clear_flag(sc->card, LV_OBJ_FLAG_SCROLLABLE);

    /* Status dot (top-right) */
    sc->status_dot = lv_obj_create(sc->card);
    lv_obj_set_size(sc->status_dot, 5, 5);
    lv_obj_set_style_radius(sc->status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(sc->status_dot, MD_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(sc->status_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sc->status_dot, 0, 0);
    lv_obj_align(sc->status_dot, LV_ALIGN_TOP_RIGHT, 0, 0);

    /* Title label */
    sc->lbl_title = lv_label_create(sc->card);
    lv_label_set_text(sc->lbl_title, title);
    lv_obj_set_style_text_font(sc->lbl_title, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(sc->lbl_title, MD_COLOR_TEXT_SEC, 0);
    lv_obj_align(sc->lbl_title, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Value label */
    sc->lbl_value = lv_label_create(sc->card);
    lv_label_set_text(sc->lbl_value, "--");
    lv_obj_set_style_text_font(sc->lbl_value, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(sc->lbl_value, MD_COLOR_TEXT, 0);
    lv_obj_align(sc->lbl_value, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    return sc;
}

void status_card_set_value(status_card_t *card, const char *value)
{
    if (card && card->lbl_value) {
        lv_label_set_text(card->lbl_value, value);
    }
}

void status_card_set_status(status_card_t *card, lv_color_t color)
{
    if (card && card->status_dot) {
        lv_obj_set_style_bg_color(card->status_dot, color, 0);
    }
}
