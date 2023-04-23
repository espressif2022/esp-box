/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "lvgl.h"
#include <stdio.h>

#include "lv_example_pub.h"
#include "lv_example_image.h"
// #include "esp32_c3_lcd_ev_board.h"

static const char *TAG = __FUNCTION__;
#define FUNC_NUM    4

typedef struct {
    char *name;
    const lv_img_dsc_t *icon_select;
    const lv_img_dsc_t *icon_unselect;
    void *layer;
} ui_menu_focus_t;

static ui_menu_focus_t mainmenu[FUNC_NUM] = {
    {"温度",    &main_temp_select,  &main_temp_unselect,  &temp_set_layer},
    {"模式",    &main_mode_select,  &main_mode_unselect,  &mode_set_layer},
    {"风速",    &main_speed_select, &main_speed_unselect, &air_speed_layer},
    {"关机",    &main_power_select, &main_power_unselect, &air_network_layer},
};

static bool air_main_layer_enter_cb(void *layer);
static bool air_main_layer_exit_cb(void *layer);
static void air_main_layer_timer_cb(lv_timer_t *tmr);

static lv_obj_t *page_parent;

static uint8_t func_focus = 0;
static lv_obj_t *obj_img_button[FUNC_NUM];

static time_out_count time_20ms, time_500ms;

lv_layer_t air_main_layer = {
    .lv_obj_name    = "air_main_layer",
    .lv_obj_parent  = NULL,
    .lv_obj_layer   = NULL,
    .lv_show_layer  = NULL,
    .enter_cb       = air_main_layer_enter_cb,
    .exit_cb        = air_main_layer_exit_cb,
    .timer_cb       = air_main_layer_timer_cb,
};

static uint32_t ui_get_num_offset(uint32_t num, int32_t max, int32_t offset)
{
    if (num >= max) {
        ESP_LOGI(TAG, "[ERROR] num should less than max");
        return num;
    }
    uint32_t i;
    if (offset >= 0) {
        i = (num + offset) % max;
    } else {
        offset = max + (offset % max);
        i = (num + offset) % max;
    }
    return i;
}

static uint32_t get_app_index(int8_t offset)
{
    return ui_get_num_offset(func_focus, FUNC_NUM, offset);
}

static void air_mainevent_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_EVENT_FOCUSED == code) {
        lv_group_set_editing(lv_group_get_default(), true);
    } else if (LV_EVENT_KEY == code) {
        uint32_t key = lv_event_get_key(e);
        if (is_time_out(&time_500ms)) {
            int8_t last_index = func_focus;
            if (LV_KEY_RIGHT == key) {
                func_focus = get_app_index(1);
            } else if (LV_KEY_LEFT == key) {
                func_focus = get_app_index(-1);
            }
            lv_img_set_src(obj_img_button[last_index], mainmenu[last_index].icon_unselect);
            lv_img_set_src(obj_img_button[get_app_index(0)], mainmenu[get_app_index(0)].icon_select);
        }
    } else if (LV_EVENT_CLICKED == code) {
        if (mainmenu[get_app_index(0)].layer) {
            lv_group_set_editing(lv_group_get_default(), false);
            ui_remove_all_objs_from_encoder_group();
            lv_func_goto_layer(mainmenu[get_app_index(0)].layer);
        }
    } else if (LV_EVENT_LONG_PRESSED == code) {
        // lv_indev_wait_release(lv_indev_get_next(NULL));
        // ui_remove_all_objs_from_encoder_group();
        // lv_func_goto_layer(&menu_layer);
    }
}

static void create_title(lv_obj_t *parent)
{
    page_parent = lv_obj_create(parent);
    lv_obj_set_size(page_parent, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(page_parent, lv_color_hex(0x212735), 0);

    lv_obj_set_style_border_width(page_parent, 0, 0);
    lv_obj_set_style_radius(page_parent, 0, 0);
    lv_obj_clear_flag(page_parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(page_parent);

    lv_obj_t *line = lv_line_create(page_parent);
    static lv_point_t line_points[] = { {10, 20}, {290, 20} };
    lv_line_set_points(line, line_points, sizeof(line_points) / sizeof(lv_point_t));
    lv_obj_set_style_line_width(line, 2, LV_PART_MAIN);
    lv_obj_set_style_line_color(line, lv_color_hex(COLOUR_GREY_4F), LV_PART_MAIN);

    lv_obj_t *img_weather = lv_img_create(page_parent);
    lv_img_set_src(img_weather, &main_air_weather);
    lv_obj_align(img_weather, LV_ALIGN_TOP_LEFT, 20, -5);

    lv_obj_t *city_temp = lv_label_create(page_parent);
    lv_obj_set_style_text_color(city_temp, lv_color_hex(COLOUR_WHITE), 0);
    lv_obj_align(city_temp, LV_ALIGN_TOP_LEFT, 50, 2);
    lv_obj_set_style_text_font(city_temp, &SourceHanSansCN_Normal_12, 0);
    lv_label_set_text(city_temp, "24℃");

    lv_obj_t *city_label = lv_label_create(page_parent);
    lv_obj_set_style_text_color(city_label, lv_color_hex(COLOUR_WHITE), 0);
    lv_obj_align(city_label, LV_ALIGN_TOP_LEFT, 80, 2);
    lv_obj_set_style_text_font(city_label, &SourceHanSansCN_Normal_12, 0);
    lv_label_set_text(city_label, "上海");

    lv_obj_t *img_wifi = lv_img_create(page_parent);
    lv_img_set_src(img_wifi, &main_wifi_ok);
    lv_obj_align(img_wifi, LV_ALIGN_TOP_RIGHT, -20, 0);
}

void ui_air_maininit(lv_obj_t *parent)
{
    create_title(parent);

    lv_obj_t *img_air = lv_img_create(page_parent);
    lv_img_set_src(img_air, &main_air_icon);
    lv_obj_align(img_air, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *page_bottom = lv_obj_create(parent);
    lv_obj_set_size(page_bottom, LV_HOR_RES, 50);
    lv_obj_set_style_border_width(page_bottom, 0, 0);
    lv_obj_set_style_radius(page_bottom, 0, 0);
    lv_obj_align(page_bottom, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(page_bottom, lv_color_hex(0x30394d), 0);
    lv_obj_set_style_bg_opa(page_bottom, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);

    lv_obj_t *page_bottom_icon = lv_obj_create(page_bottom);
    lv_obj_remove_style_all(page_bottom_icon);
    lv_obj_set_size(page_bottom_icon, LV_HOR_RES - 30, 50);

    for (int i = 0; i < FUNC_NUM; i++) {
        obj_img_button[i] = lv_img_create(page_bottom_icon);
        lv_img_set_src(obj_img_button[i], (i == func_focus) ? mainmenu[i].icon_select : mainmenu[i].icon_unselect);
    }

    static lv_coord_t grid_clock_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_clock_row_dsc[] = {1, 40, LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(page_bottom_icon, grid_clock_col_dsc, grid_clock_row_dsc);
    lv_obj_set_grid_cell(obj_img_button[0], LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_grid_cell(obj_img_button[1], LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_grid_cell(obj_img_button[2], LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_grid_cell(obj_img_button[3], LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 1, 1);


    lv_obj_add_event_cb(page_parent, air_mainevent_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(page_parent, air_mainevent_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(page_parent, air_mainevent_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(page_parent, air_mainevent_cb, LV_EVENT_CLICKED, NULL);
    ui_add_obj_to_encoder_group(page_parent);
}


static bool air_main_layer_enter_cb(void *layer)
{
    bool ret = false;

    LV_LOG_USER("");
    lv_layer_t *create_layer = layer;
    if (NULL == create_layer->lv_obj_layer) {
        ret = true;
        create_layer->lv_obj_layer = lv_obj_create(lv_scr_act());
        lv_obj_remove_style_all(create_layer->lv_obj_layer);
        lv_obj_set_size(create_layer->lv_obj_layer, LV_HOR_RES, LV_VER_RES);

        ui_air_maininit(create_layer->lv_obj_layer);
        set_time_out(&time_20ms, 20);
        set_time_out(&time_500ms, 200);
    }

    return ret;
}

static bool air_main_layer_exit_cb(void *layer)
{
    LV_LOG_USER("");
    return true;
}

static void air_main_layer_timer_cb(lv_timer_t *tmr)
{
    feed_clock_time();

    if (is_time_out(&time_20ms)) {
    }
}
