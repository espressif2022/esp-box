/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#ifndef LV_EXAMPLE_IMAGE_H
#define LV_EXAMPLE_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
LV_IMG_DECLARE(icon_clock);
LV_IMG_DECLARE(icon_fans);
LV_IMG_DECLARE(icon_light);
LV_IMG_DECLARE(icon_player);
LV_IMG_DECLARE(icon_weather);
LV_IMG_DECLARE(icon_washing);
LV_IMG_DECLARE(icon_thermostat);
LV_IMG_DECLARE(icon_washing_ns);
LV_IMG_DECLARE(icon_thermostat_ns);
LV_IMG_DECLARE(icon_light_ns);

LV_IMG_DECLARE(img_weather);
LV_IMG_DECLARE(img_bg);
LV_IMG_DECLARE(espressif_logo);

LV_IMG_DECLARE(light_close_bg)
LV_IMG_DECLARE(light_close_pwm)
LV_IMG_DECLARE(light_close_status)
LV_IMG_DECLARE(light_cool_100)
LV_IMG_DECLARE(light_cool_25)
LV_IMG_DECLARE(light_cool_50)
LV_IMG_DECLARE(light_cool_75)
LV_IMG_DECLARE(light_cool_bg)
LV_IMG_DECLARE(light_warm_100)
LV_IMG_DECLARE(light_warm_25)
LV_IMG_DECLARE(light_warm_50)
LV_IMG_DECLARE(light_warm_75)
LV_IMG_DECLARE(light_warm_bg)

LV_IMG_DECLARE(light_pwm_00)
LV_IMG_DECLARE(light_pwm_25)
LV_IMG_DECLARE(light_pwm_50)
LV_IMG_DECLARE(light_pwm_75)
LV_IMG_DECLARE(light_pwm_100)

LV_IMG_DECLARE(img_washing_bg);
LV_IMG_DECLARE(img_washing_wave1);
LV_IMG_DECLARE(img_washing_wave2);
LV_IMG_DECLARE(img_washing_bubble1);
LV_IMG_DECLARE(img_washing_bubble2);
LV_IMG_DECLARE(img_washing_stand);
LV_IMG_DECLARE(img_washing_shirt);
LV_IMG_DECLARE(img_washing_underwear);
LV_IMG_DECLARE(wash_underwear1)
LV_IMG_DECLARE(wash_underwear2)
LV_IMG_DECLARE(wash_shirt)
LV_IMG_DECLARE(wash_basic)
LV_IMG_DECLARE(wash_blouse)
LV_IMG_DECLARE(wash_briefs)

LV_IMG_DECLARE(AC_BG)
LV_IMG_DECLARE(AC_temper)
LV_IMG_DECLARE(AC_unit)

LV_IMG_DECLARE(standby_eye_left)
LV_IMG_DECLARE(standby_eye_right)
LV_IMG_DECLARE(standby_eye_1)
LV_IMG_DECLARE(standby_eye_2)
LV_IMG_DECLARE(standby_eye_close)
LV_IMG_DECLARE(standby_face)
LV_IMG_DECLARE(standby_mouth_2)
LV_IMG_DECLARE(standby_eye_1_fade)
LV_IMG_DECLARE(standby_eye_3)
LV_IMG_DECLARE(standby_eye_open)
LV_IMG_DECLARE(standby_mouth_1)

LV_IMG_DECLARE(language_bg)
LV_IMG_DECLARE(language_bg_dither)
LV_IMG_DECLARE(language_select)
LV_IMG_DECLARE(language_unselect)


LV_IMG_DECLARE(main_air_icon);
LV_IMG_DECLARE(main_mode_unselect);
LV_IMG_DECLARE(main_power_unselect);
LV_IMG_DECLARE(main_speed_unselect);
LV_IMG_DECLARE(main_temp_unselect);
LV_IMG_DECLARE(main_mode_select);
LV_IMG_DECLARE(main_power_select);
LV_IMG_DECLARE(main_speed_select);
LV_IMG_DECLARE(main_temp_select);
LV_IMG_DECLARE(main_wifi_fail);  
LV_IMG_DECLARE(main_wifi_ok);
LV_IMG_DECLARE(main_air_weather);

LV_IMG_DECLARE(speed_air_fan);
LV_IMG_DECLARE(speed_fan_bg);
LV_IMG_DECLARE(speed_fan_select_3);
LV_IMG_DECLARE(speed_fan_unselect_3);
LV_IMG_DECLARE(speed_fan_select_2);
LV_IMG_DECLARE(speed_fan_unselect_2);
LV_IMG_DECLARE(speed_fan_select_1);
LV_IMG_DECLARE(speed_fan_unselect_1);
    
LV_IMG_DECLARE(mode_btn_cool_select);  
LV_IMG_DECLARE(mode_btn_warm_select);     
LV_IMG_DECLARE(mode_cool_bg);     
LV_IMG_DECLARE(mode_icon_warm); 
LV_IMG_DECLARE(mode_btn_cool_unselect);   
LV_IMG_DECLARE(mode_btn_warm_unselect);   
LV_IMG_DECLARE(mode_icon_cool);   
LV_IMG_DECLARE(mode_warm_bg); 

LV_IMG_DECLARE(temp_add);
LV_IMG_DECLARE(temp_add_select);
LV_IMG_DECLARE(temp_add_unselect);
LV_IMG_DECLARE(temp_dec);
LV_IMG_DECLARE(temp_dec_select);
LV_IMG_DECLARE(temp_dec_unselect);
LV_IMG_DECLARE(temp_select);
LV_IMG_DECLARE(temp_unselect);

/********************************
 * font
********************************/
/*012345678926℃*/
LV_FONT_DECLARE(Helvetica25);
LV_FONT_DECLARE(Helvetica65);

/* 温度模式风速关机上海网络输入的密码制冷制暖 */
LV_FONT_DECLARE(SourceHanSansCN_Normal_12);

/*0x20-0x7F*/
LV_FONT_DECLARE(Helvetica_Neue14);


/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_EXAMPLE_IMAGE_H*/