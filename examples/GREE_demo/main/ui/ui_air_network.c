/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "lvgl.h"
#include <stdio.h>

#include "esp_wifi.h"
#include "app_wifi.h"
#include "settings.h"

#include "lv_example_pub.h"
#include "lv_example_image.h"

static const char *TAG = __FUNCTION__;
#define FUNC_NUM    3

typedef struct {
    char *name;
    const lv_img_dsc_t *icon_select;
    const lv_img_dsc_t *icon_unselect;
    void *layer;
} ui_menu_focus_t;

static bool air_network_layer_enter_cb(void *layer);
static bool air_network_layer_exit_cb(void *layer);
static void air_network_layer_timer_cb(lv_timer_t *tmr);

static lv_obj_t *page_parent;
static lv_obj_t *currentButton[DEFAULT_SCAN_LIST_SIZE], *list_wifi = NULL;

static uint8_t func_focus = 0;

static sys_param_t *sys_set;
static time_out_count time_fan_speed, time_500ms;

lv_layer_t air_network_layer = {
    .lv_obj_name    = "air_network_layer",
    .lv_obj_parent  = NULL,
    .lv_obj_layer   = NULL,
    .lv_show_layer  = NULL,
    .enter_cb       = air_network_layer_enter_cb,
    .exit_cb        = air_network_layer_exit_cb,
    .timer_cb       = air_network_layer_timer_cb,
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
    return ui_get_num_offset(func_focus, scan_info_result.ap_count, offset);
}

static void event_handler_up(uint8_t index)
{
    lv_obj_t * child = lv_obj_get_child(currentButton[index], 0);
    ESP_LOGI(TAG, "event_handler_up:%s", lv_label_get_text(child));
    // lv_obj_scroll_to_view(currentButton[index], LV_ANIM_ON);

    for (int i = 0; i < scan_info_result.ap_count + 1; i++) {
        if(index == i){
            lv_obj_add_state(currentButton[i], LV_STATE_CHECKED);
        }
        else{
            lv_obj_clear_state(currentButton[i], LV_STATE_CHECKED);
        }
    }
}

static void air_networkevent_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_EVENT_FOCUSED == code) {
        lv_group_set_editing(lv_group_get_default(), true);
    } else if (LV_EVENT_KEY == code) {
        uint32_t key = lv_event_get_key(e);
        if (is_time_out(&time_500ms)) {
            int8_t last_index = func_focus;

            // bsp_display_lock(0);
            if (LV_KEY_RIGHT == key) {
                func_focus = get_app_index(1);
            } else if (LV_KEY_LEFT == key) {
                func_focus = get_app_index(-1);
            }
            // bsp_display_unlock();
            event_handler_up(func_focus);
            ESP_LOGI(TAG, "func_focus:%d", func_focus);
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

    lv_obj_t *line = lv_line_create(page_parent);
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
    lv_label_set_text(select_name, "网络");

    lv_obj_t *img_wifi = lv_img_create(page_parent);
    lv_img_set_src(img_wifi, &main_wifi_ok);
    lv_obj_align(img_wifi, LV_ALIGN_TOP_RIGHT, -20, 0);
}

void set_button_list_style(lv_obj_t *parent)
{
    lv_obj_set_size(parent, LV_PCT(95), 70);
    lv_obj_set_style_bg_color(parent, lv_color_hex(COLOUR_GREY_2F), LV_PART_MAIN);
    lv_obj_set_style_text_color(parent, lv_color_hex(COLOUR_WHITE), 0);
    lv_obj_set_style_border_color(parent, lv_color_hex(COLOUR_GREY_4F), 0);
    // lv_obj_set_style_transform_width(btn, LV_PCT(90), 0);
    lv_obj_set_style_border_width(parent, 1, 0);
    lv_obj_set_style_border_side(parent, LV_BORDER_SIDE_BOTTOM, 0);
}

static void update_wifi_list(lv_obj_t *parent)
{
    lv_obj_t *label_item;

    if (NULL == parent) {
        return;
    }

    if (WIFI_SCAN_BUSY == scan_info_result.scan_done) {
        if (false == lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN);
        }

        if (lv_obj_get_child_cnt(parent)) {
            lv_obj_clean(parent);
        }
    } else if ((WIFI_SCAN_RENEW == scan_info_result.scan_done) || (WIFI_SCAN_UPDATE == scan_info_result.scan_done)) {
        if (true == lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(parent, LV_OBJ_FLAG_HIDDEN);
        }

        if (WIFI_SCAN_UPDATE == scan_info_result.scan_done) {
            if (lv_obj_get_child_cnt(parent)) {
                lv_obj_clean(parent);
            }
        }

        ESP_LOGI(TAG, "update wifi list");
        scan_info_result.scan_done = WIFI_SCAN_IDLE;

        for (int i = 0; i < scan_info_result.ap_count; i++) {
            if ((0 == strcmp((char *)sys_set->ssid, (char *)scan_info_result.ap_info[i].ssid)) && (i > 0)) {
                uint8_t replace_ssid[32] = {0};
                memcpy(replace_ssid, scan_info_result.ap_info[0].ssid, sizeof(replace_ssid));
                memcpy(scan_info_result.ap_info[0].ssid,
                       scan_info_result.ap_info[i].ssid, sizeof(replace_ssid));
                memcpy(scan_info_result.ap_info[i].ssid, replace_ssid, sizeof(replace_ssid));
                ESP_LOGI(TAG, "replace[%d->0:%s]", i, scan_info_result.ap_info[i].ssid);
            }
        }

        for (int i = 0; i < scan_info_result.ap_count + 1; i++) {

            currentButton[i] = lv_list_add_btn(parent, NULL, NULL);
            lv_obj_remove_style_all(currentButton[i]);
            set_button_list_style(currentButton[i]);
            lv_obj_set_size(currentButton[i], LV_PCT(95), 35);
            lv_obj_add_flag(currentButton[i], LV_OBJ_FLAG_EVENT_BUBBLE);
            lv_obj_set_style_bg_opa(currentButton[i], LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
            lv_obj_set_style_bg_color(currentButton[i], lv_color_hex(0x415068), LV_PART_MAIN | LV_STATE_CHECKED);

            lv_obj_add_state(currentButton[0], LV_STATE_CHECKED);
            
            if (i >= scan_info_result.ap_count) {
                lv_obj_set_style_border_side(currentButton[i], LV_BORDER_SIDE_NONE, 0);
                continue;
            }
            // lv_obj_add_event_cb(btn, wifi_list_event_handler, LV_EVENT_SHORT_CLICKED, (void *)i);

            label_item = lv_label_create(currentButton[i]);
            lv_obj_set_style_text_font(label_item, &Helvetica_Neue14, 0);

            if (0 == strcmp((char *)scan_info_result.ap_info[i].ssid, (char *)sys_set->ssid)) {

                char ssid_conected[32 + 10];
                memset(ssid_conected, 0, sizeof(ssid_conected));

                WiFi_Connect_Status status = wifi_connected_already();
                if (WIFI_STATUS_CONNECTING == status) {
                    sprintf(ssid_conected, "%s %s", scan_info_result.ap_info[i].ssid, "连接中");
                } else if (WIFI_STATUS_CONNECTED_FAILED == status) {
                    sprintf(ssid_conected, "%s %s", scan_info_result.ap_info[i].ssid, "未连接");
                } else {
                    sprintf(ssid_conected, "%s %s", scan_info_result.ap_info[i].ssid, "已连接");
                }

                lv_label_set_text(label_item, ssid_conected);
                lv_obj_set_style_text_color(label_item, lv_color_hex(COLOUR_YELLOW), 0);
            } else {
                lv_obj_set_style_text_color(label_item, lv_color_hex(COLOUR_WHITE), 0);
                lv_label_set_text(label_item, (char *)scan_info_result.ap_info[i].ssid);
            }
            lv_obj_align(label_item, LV_ALIGN_LEFT_MID, 10, 0);

            lv_obj_t *label_enter = lv_label_create(currentButton[i]);
            lv_obj_set_style_text_font(label_enter, &lv_font_montserrat_16, 0);
            lv_label_set_text(label_enter, LV_SYMBOL_RIGHT);
            lv_obj_set_style_text_color(label_enter, lv_color_hex(COLOUR_WHITE), 0);
            lv_obj_align(label_enter, LV_ALIGN_RIGHT_MID, 0, 0);
        }
    }
}

void ui_air_networkinit(lv_obj_t *parent)
{
    create_title(parent);

    list_wifi = lv_list_create(parent);
    lv_obj_set_size(list_wifi, 300, 180);
    lv_obj_set_style_border_width(list_wifi, 0, 0);
    lv_obj_set_style_radius(list_wifi, 30, 0);
    lv_obj_align(list_wifi, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(list_wifi, lv_color_hex(0x30394d), 0);

    lv_obj_add_flag(list_wifi, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_clear_flag(list_wifi, LV_OBJ_FLAG_CLICKABLE);

    scan_info_result.scan_done = WIFI_SCAN_RENEW;
    update_wifi_list(list_wifi);

    lv_obj_add_event_cb(parent, air_networkevent_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(parent, air_networkevent_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(parent, air_networkevent_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(parent, air_networkevent_cb, LV_EVENT_CLICKED, NULL);
    ui_add_obj_to_encoder_group(parent);
}


static bool air_network_layer_enter_cb(void *layer)
{
    bool ret = false;

    LV_LOG_USER("");
    // send_network_event(NET_EVENT_SCAN);
    sys_set = settings_get_parameter();

    lv_layer_t *create_layer = layer;

    if (NULL == create_layer->lv_obj_layer) {
        ret = true;
        create_layer->lv_obj_layer = lv_obj_create(lv_scr_act());
        lv_obj_remove_style_all(create_layer->lv_obj_layer);
        lv_obj_set_size(create_layer->lv_obj_layer, LV_HOR_RES, LV_VER_RES);

        ui_air_networkinit(create_layer->lv_obj_layer);

        func_focus = 0;
        set_time_out(&time_fan_speed, 180);
        set_time_out(&time_500ms, 200);
    }

    return ret;
}

static bool air_network_layer_exit_cb(void *layer)
{
    LV_LOG_USER("");
    return true;
}

static void air_network_layer_timer_cb(lv_timer_t *tmr)
{
    static uint16_t fan_speed;
    feed_clock_time();

    if (is_time_out(&time_fan_speed)) {
        update_wifi_list(list_wifi);
    }
}
