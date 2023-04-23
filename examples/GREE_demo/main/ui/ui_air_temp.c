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
#define FUNC_NUM    2

typedef struct {
    const lv_img_dsc_t *icon_select;
    const lv_img_dsc_t *icon_unselect;
} ui_menu_focus_t;

static ui_menu_focus_t speed_menu[FUNC_NUM] = {
    {&temp_dec_select,  &temp_dec_unselect},
    {&temp_add_select,  &temp_add_unselect},
};

static bool temp_set_layer_enter_cb(void *layer);
static bool temp_set_layer_exit_cb(void *layer);
static void temp_set_layer_timer_cb(lv_timer_t *tmr);

static lv_obj_t *page_parent;

static uint8_t func_focus = 0;
static uint8_t temp_value_set = 26;

static lv_obj_t *obj_img_button[FUNC_NUM];
static lv_obj_t *label_temp_set[3];

static time_out_count time_fan_speed, time_500ms;

lv_layer_t temp_set_layer = {
    .lv_obj_name    = "temp_set_layer",
    .lv_obj_parent  = NULL,
    .lv_obj_layer   = NULL,
    .lv_show_layer  = NULL,
    .enter_cb       = temp_set_layer_enter_cb,
    .exit_cb        = temp_set_layer_exit_cb,
    .timer_cb       = temp_set_layer_timer_cb,
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

void temp_set_show()
{
    uint8_t temp_buffer[10];

    sprintf(temp_buffer, "%d", temp_value_set);
    lv_label_set_text(label_temp_set[1], temp_buffer);

    sprintf(temp_buffer, "%d℃", (temp_value_set -1));
    lv_label_set_text(label_temp_set[0], temp_buffer);

    sprintf(temp_buffer, "%d℃", (temp_value_set +1));
    lv_label_set_text(label_temp_set[2], temp_buffer);
}

static void temp_setevent_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_EVENT_FOCUSED == code) {
        lv_group_set_editing(lv_group_get_default(), true);
    } else if (LV_EVENT_KEY == code) {
        uint32_t key = lv_event_get_key(e);

        if (is_time_out(&time_500ms)) {
            int8_t last_index = func_focus;
            if (LV_KEY_RIGHT == key) {
                if(func_focus == 1){//focus +, temp ++
                    temp_value_set++;
                }else{
                    func_focus = get_app_index(1);
                }
            } else if (LV_KEY_LEFT == key) {
                if(func_focus == 0){//focus -, temp --
                    temp_value_set--;
                }else{
                    func_focus = get_app_index(-1);
                }
            }

            lv_img_set_src(obj_img_button[last_index], speed_menu[last_index].icon_unselect);
            lv_img_set_src(obj_img_button[get_app_index(0)], speed_menu[get_app_index(0)].icon_select);
            temp_set_show();
        }
    } else if (LV_EVENT_CLICKED == code) {
    } else if (LV_EVENT_LONG_PRESSED == code) {
        lv_indev_wait_release(lv_indev_get_next(NULL));
        ui_remove_all_objs_from_encoder_group();
        lv_func_goto_layer(&air_main_layer);
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
    lv_obj_align(page_parent, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* line = lv_line_create(page_parent);
    static lv_point_t line_points[] = { {10, 20}, {290, 20} };
    lv_line_set_points(line, line_points, sizeof(line_points) / sizeof(lv_point_t));
    lv_obj_set_style_line_width(line, 2, LV_PART_MAIN);
    lv_obj_set_style_line_color(line, lv_color_hex(COLOUR_GREY_4F), LV_PART_MAIN);

    lv_obj_t *time_label = lv_label_create(page_parent);
    lv_obj_set_style_text_color(time_label, lv_color_hex(COLOUR_WHITE), 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_LEFT, 20, 2);
    lv_obj_set_style_text_font(time_label, &SourceHanSansCN_Normal_12, 0);
    lv_label_set_text(time_label, "14:28");

    lv_obj_t *select_name = lv_label_create(page_parent);
    lv_obj_set_style_text_color(select_name, lv_color_hex(COLOUR_WHITE), 0);
    lv_obj_align(select_name, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_text_font(select_name, &SourceHanSansCN_Normal_12, 0);
    lv_label_set_text(select_name, "温度");

    lv_obj_t *img_wifi = lv_img_create(page_parent);
    lv_img_set_src(img_wifi, &main_wifi_ok);
    lv_obj_align(img_wifi, LV_ALIGN_TOP_RIGHT, -20, 0);
}

void ui_temp_setinit(lv_obj_t *parent)
{
    create_title(parent);

    lv_obj_t *page_mid = lv_obj_create(parent);
    lv_obj_set_size(page_mid, 300, 100);
    lv_obj_set_style_border_width(page_mid, 0, 0);
    lv_obj_set_style_radius(page_mid, 30, 0);
    lv_obj_align(page_mid, LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_set_style_bg_color(page_mid, lv_color_hex(0x30394d), 0);
    lv_obj_set_style_bg_opa(page_mid, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);

    label_temp_set[1] = lv_label_create(page_mid);
    lv_obj_set_style_text_color(label_temp_set[1], lv_color_hex(COLOUR_WHITE), 0);
    lv_obj_align(label_temp_set[1], LV_ALIGN_CENTER, -10, 0);
    lv_obj_set_style_text_font(label_temp_set[1], &Helvetica65, 0);

    lv_obj_t *img_mode_icon = lv_img_create(page_mid);
    lv_img_set_src(img_mode_icon, &mode_icon_cool);
    lv_obj_align_to(img_mode_icon, label_temp_set[1], LV_ALIGN_OUT_RIGHT_BOTTOM, -10, 0);

    lv_obj_t *temp_unit_label = lv_label_create(page_mid);
    lv_obj_set_style_text_color(temp_unit_label, lv_color_hex(COLOUR_WHITE), 0);
    lv_obj_set_style_text_font(temp_unit_label, &Helvetica25, 0);
    lv_label_set_text(temp_unit_label, "℃");
    lv_obj_align_to(temp_unit_label, label_temp_set[1], LV_ALIGN_OUT_RIGHT_TOP, -10, 0);

    label_temp_set[0] = lv_label_create(page_mid);
    lv_obj_set_style_text_color(label_temp_set[0], lv_color_hex(0x515968), 0);
    lv_obj_align(label_temp_set[0], LV_ALIGN_CENTER, -100, 0);
    lv_obj_set_style_text_font(label_temp_set[0], &Helvetica25, 0);

    label_temp_set[2] = lv_label_create(page_mid);
    lv_obj_set_style_text_color(label_temp_set[2], lv_color_hex(0x515968), 0);
    lv_obj_align(label_temp_set[2], LV_ALIGN_CENTER, 100, 0);
    lv_obj_set_style_text_font(label_temp_set[2], &Helvetica25, 0);

    temp_set_show();

    lv_obj_t *page_bottom = lv_obj_create(parent);
    lv_obj_set_size(page_bottom, 300, 50);
    lv_obj_set_style_border_width(page_bottom, 0, 0);
    lv_obj_set_style_radius(page_bottom, 30, 0);
    lv_obj_align(page_bottom, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(page_bottom, lv_color_hex(0x30394d), 0);
    lv_obj_set_style_bg_opa(page_bottom, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);

    for(int i = 0; i< FUNC_NUM; i++){
        obj_img_button[i] = lv_img_create(page_bottom);
        lv_img_set_src(obj_img_button[i], (i == func_focus) ? speed_menu[i].icon_select : speed_menu[i].icon_unselect);
    }

    static lv_coord_t grid_clock_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_clock_row_dsc[] = {30, 20, LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(page_bottom, grid_clock_col_dsc, grid_clock_row_dsc);
    lv_obj_set_grid_cell(obj_img_button[0], LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_grid_cell(obj_img_button[1], LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);


    lv_obj_add_event_cb(page_parent, temp_setevent_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(page_parent, temp_setevent_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(page_parent, temp_setevent_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(page_parent, temp_setevent_cb, LV_EVENT_CLICKED, NULL);
    ui_add_obj_to_encoder_group(page_parent);
}


static bool temp_set_layer_enter_cb(void *layer)
{
    bool ret = false;

    LV_LOG_USER("");
    lv_layer_t *create_layer = layer;
    if (NULL == create_layer->lv_obj_layer) {
        ret = true;
        create_layer->lv_obj_layer = lv_obj_create(lv_scr_act());
        lv_obj_remove_style_all(create_layer->lv_obj_layer);
        lv_obj_set_size(create_layer->lv_obj_layer, LV_HOR_RES, LV_VER_RES);

        ui_temp_setinit(create_layer->lv_obj_layer);

        func_focus = 0;
        set_time_out(&time_fan_speed, 180);
        set_time_out(&time_500ms, 200);
    }

    return ret;
}

static bool temp_set_layer_exit_cb(void *layer)
{
    LV_LOG_USER("");
    return true;
}

static void temp_set_layer_timer_cb(lv_timer_t *tmr)
{
    static uint16_t fan_speed;
    feed_clock_time();

    if (is_time_out(&time_fan_speed)) {
    }
}
