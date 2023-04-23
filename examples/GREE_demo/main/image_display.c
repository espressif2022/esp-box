/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <dirent.h>

#include "nvs_flash.h"
#include "settings.h"
#include "app_wifi.h"
#include "app_weather.h"

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "lv_example_pub.h"

static const char *TAG = "main";

void app_main(void)
{
    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(settings_read_parameter_from_nvs());

    /* Initialize I2C (for touch and audio) */
    bsp_i2c_init();

    /* Initialize display and LVGL */
    bsp_display_start();

    /* Set display brightness to 100% */
    bsp_display_backlight_on();

    /* Mount SPIFFS */
    bsp_spiffs_mount();

    // image_display();
    ESP_LOGI(TAG, "Display LVGL demo");
    ui_obj_to_encoder_init();
    bsp_display_lock(0);
    lv_create_home(&air_main_layer);
    lv_create_clock(NULL, TIME_ENTER_CLOCK_2MIN);
    bsp_display_unlock();

    app_weather_start();
    app_network_start();
}
