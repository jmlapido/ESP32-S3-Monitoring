#include "ui_theme.h"
#include "esp_log.h"

static const char *TAG = "ui_theme";

void ui_theme_init(void)
{
    ESP_LOGI(TAG, "Applying dark theme");

    lv_display_t *disp = lv_display_get_default();
    lv_theme_t *th = lv_theme_default_init(
        disp,
        MD_COLOR_PRIMARY,
        MD_COLOR_SUCCESS,
        true,   /* dark mode */
        &lv_font_montserrat_14
    );
    lv_display_set_theme(disp, th);

    /* Set default screen background */
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, MD_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    ESP_LOGI(TAG, "Dark theme applied");
}
