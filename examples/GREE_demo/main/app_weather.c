/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "app_weather.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_client.h"

static const char *TAG = "app_weather";

#define LOG_TRACE(...)  ESP_LOGI(TAG, ##__VA_ARGS__)
#define CHECK(a, str, ret_val) do { \
        if (!(a)) { \
            ESP_LOGE(TAG,"%s(%d): %s", __FUNCTION__, __LINE__, str); \
            return (ret_val); \
        } \
    } while(0)


#define MAX_HTTP_RECV_BUFFER (1024)

/**
 * @brief User Agent string
 *
 */
#define _UA_                            "esp32_S3_86_box"
/**
 * @brief key string, get it at https://console.qweather.com/#/apps
 *
 */
#define QWEATHER_REQUEST_KEY_LEN        (32)
#define CONFIG_QWEATHER_REQUEST_KEY     "fefdad78ca2542ec93558240b30bb289"
#define _KEY_VALUE_                     CONFIG_QWEATHER_REQUEST_KEY
#define _KEY_                           "key=" _KEY_VALUE_

/**
 * @brief HTTPS request param. You can found location code at QWeather API page.
 *      And please keep gzip=n, otherwise you need to unzip the data by your self and use more Flash and RAM.
 *
 */
#define LOCATION_SHANGHAI               "101020100"
#define LOCATION_BEIJING                "101010100"
#define LOCATION_SHENZHEN               "101280601"

#define _OPTION_                        "location=101020100&gzip=n&lang=zh"
#define _OPTION_MULTI                   "location=%s&gzip=n&lang=en"

/**
 * @brief See more at https://dev.qweather.com/docs/api/
 *
 */
#define WEATHER_SERVER      "devapi.qweather.com"
#define WEB_URL_NOW         "https://" WEATHER_SERVER "/v7/weather/now?" _OPTION_MULTI "&" _KEY_
#define WEB_URL_AIR         "https://" WEATHER_SERVER "/v7/air/now?"     _OPTION_ "&" _KEY_

/**
 * @brief Macro to build https request URL
 *
 */
#define URL_CONSTRUCT(WEB_URL, WEB_SERVER) \
    "GET "          WEB_URL     " HTTP/1.1\r\n" \
    "Host: "        WEB_SERVER  "\r\n" \
    "User-Agent: "  _UA_        "\r\n" \
    "\r\n"

typedef esp_err_t (*parse_func_t)(char *, location_num_t);
static esp_err_t app_weather_parse_now(char *buffer, location_num_t location);
static esp_err_t app_weather_parse_air(char *buffer, location_num_t location);

static parse_func_t parse_func[] = {
    app_weather_parse_now,
    app_weather_parse_air,
};

static https_request_t request_list[] = {
    {
        .url = NULL,
        .rsp = NULL,
        .rsp_size = 4096,
        .req = NULL
    },
    {
        .url = WEB_URL_AIR,
        .rsp = NULL,
        .rsp_size = 4096,
        .req = URL_CONSTRUCT(WEB_URL_AIR, WEATHER_SERVER)
    },
};

static air_info_t *air_info = NULL;
static weather_info_t *weather_info[LOCATION_NUM_MAX];


esp_err_t response_handler(esp_http_client_event_t *evt)
{
    static char *data = NULL; // Initialize data to NULL
    static int data_len = 0; // Initialize data to NULL

    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        printf("HTTP_EVENT_ERROR\n");
        break;

    case HTTP_EVENT_ON_CONNECTED:
        printf("HTTP_EVENT_ON_CONNECTED\n");
        break;

    case HTTP_EVENT_HEADER_SENT:
        printf("HTTP_EVENT_HEADER_SENT\n");
        break;

    case HTTP_EVENT_ON_HEADER:
        if (evt->data_len) {
            printf("HTTP_EVENT_ON_HEADER\n");
            printf("%.*s", evt->data_len, (char *)evt->user_data);
        }
        break;

    case HTTP_EVENT_ON_DATA:
        // printf("HTTP_EVENT_ON_DATA (%d +)%d\n", data_len, evt->data_len);
        printf("\nRaw Response: data length: (%d +)%d: %.*s\n", data_len, evt->data_len, evt->data_len, (char *)evt->data);
        for (int i  = 0; i < evt->data_len; i++) {
            printf("%02x ", *((char *)evt->data + i));
        }
        printf("\r\n");
        // data = heap_caps_realloc(data, data_len + evt->data_len + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        // if (data == NULL) {
        //     printf("data realloc failed\n");
        //     free(data);
        //     data = NULL;
        //     break;
        // }
        // memcpy(data + data_len, (char *)evt->data, evt->data_len);
        // data_len += evt->data_len;
        // data[data_len] = '\0';
        break;

    case HTTP_EVENT_ON_FINISH:
        printf("HTTP_EVENT_ON_FINISH\n");
        break;

    case HTTP_EVENT_DISCONNECTED:
        printf("HTTP_EVENT_DISCONNECTED\n");
        break;

    default:
        break;
    }

    return ESP_OK;
}

esp_err_t app_weather_request(location_num_t location)
{
    char *rx_msg = NULL;
    char *cityID = NULL;

    for (int i = 0; i < 1; i++) {
        if (0 == i) {
            request_list[0].url = heap_caps_malloc(200, MALLOC_CAP_SPIRAM);
            request_list[0].req = heap_caps_malloc(200, MALLOC_CAP_SPIRAM);
            CHECK(request_list[i].url, "Failed allocate mem", ESP_ERR_NO_MEM);
            CHECK(request_list[i].req, "Failed allocate mem", ESP_ERR_NO_MEM);

            switch (location) {
            case LOCATION_NUM_SHANGHAI:
                cityID = LOCATION_SHANGHAI;
                break;
            case LOCATION_NUM_BEIJING:
                cityID = LOCATION_BEIJING;
                break;
            case LOCATION_NUM_SHENZHEN:
                cityID = LOCATION_SHENZHEN;
                break;
            default:
                cityID = LOCATION_SHANGHAI;
                break;
            }
            sprintf(request_list[0].url, WEB_URL_NOW, cityID);
            sprintf(request_list[0].req, URL_CONSTRUCT(WEB_URL_NOW, WEATHER_SERVER), cityID);
        }

        esp_http_client_config_t config = {
            .url = request_list[0].url,
            .method = HTTP_METHOD_GET,
            .event_handler = response_handler,
            .buffer_size = MAX_HTTP_RECV_BUFFER,
            .timeout_ms = 30000,
        };

#if 0
        printf("url:%s\r\n", request_list[0].url);

        // Set the headers
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_header(client, "Host", WEATHER_SERVER);
        esp_http_client_set_header(client, "User-Agent", _UA_);
        esp_http_client_set_header(client, "Accept-Encoding", "deflate");
        esp_http_client_set_header(client, "Cache-Control", "no-cache");
        esp_http_client_set_header(client, "Accept", "*/*");

        // Send the request
        esp_err_t err = esp_http_client_perform(client);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "HTTP POST request failed: %s\n", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);

        if (request_list[i].rsp) {
            heap_caps_free(request_list[i].rsp);
        }

        if (0 == i) {
            heap_caps_free(request_list[i].url);
            heap_caps_free(request_list[i].req);
        }
        return err;
#endif

        request_list[i].rsp = heap_caps_malloc(request_list[i].rsp_size, MALLOC_CAP_SPIRAM);
        CHECK(request_list[i].rsp, "Failed allocate mem", ESP_ERR_NO_MEM);

        if (ESP_OK == https_request(&request_list[i])) {
            https_request_get_msg_body(request_list[i].rsp, &rx_msg);
            if (NULL != rx_msg) {
                printf("rx_msg: \r\n%s\r\n", rx_msg);
                parse_func[i](rx_msg, location);
            }
        }

        if (request_list[i].rsp) {
            heap_caps_free(request_list[i].rsp);
        }

        if (0 == i) {
            heap_caps_free(request_list[i].url);
            heap_caps_free(request_list[i].req);
        }
    }

    return ESP_OK;
}

esp_err_t app_weather_start(void)
{
    CHECK(QWEATHER_REQUEST_KEY_LEN == strlen(CONFIG_QWEATHER_REQUEST_KEY),
          "Invalid QWeather request key.\n"
          "Please register at https://console.qweather.com/#/apps",
          ESP_ERR_INVALID_ARG);

    // app_weather_get_air_info(NULL);

    for (int i = 0; i < LOCATION_NUM_MAX; i++) {
        if (NULL == weather_info[i]) {
            weather_info[i] = heap_caps_malloc(sizeof(weather_info_t), MALLOC_CAP_SPIRAM);
            CHECK(weather_info[i], "Failed allocate mem", ESP_ERR_NO_MEM);

            weather_info[i]->describe = heap_caps_malloc(16, MALLOC_CAP_SPIRAM);
            CHECK(weather_info[i]->describe, "Failed allocate mem", ESP_ERR_NO_MEM);

            weather_info[i]->temp = 20;
            weather_info[i]->icon_code = 104;
            weather_info[i]->humid = 80;
            strcpy(weather_info[i]->describe, "N/A");
        }
    }

    return ESP_OK;
}

static esp_err_t app_weather_parse_now(char *buffer, location_num_t location)
{
    cJSON *json = cJSON_Parse(buffer);
    cJSON *json_now = NULL;

    if (NULL != json) {
        json_now = cJSON_GetObjectItem(json, "now");
        if (NULL != json_now) {
            cJSON *json_item_temp = cJSON_GetObjectItem(json_now, "temp");
            cJSON *json_item_icon = cJSON_GetObjectItem(json_now, "icon");
            cJSON *json_item_text = cJSON_GetObjectItem(json_now, "text");
            cJSON *json_item_humidity = cJSON_GetObjectItem(json_now, "humidity");

            ESP_LOGI(TAG, "Temp : [%s]", json_item_temp->valuestring);
            ESP_LOGI(TAG, "Icon : [%s]", json_item_icon->valuestring);
            ESP_LOGI(TAG, "Text : [%s]", json_item_text->valuestring);
            ESP_LOGI(TAG, "Humid: [%s]", json_item_humidity->valuestring);

            weather_info[location]->temp = atoi(json_item_temp->valuestring);
            weather_info[location]->icon_code = atoi(json_item_icon->valuestring);
            weather_info[location]->humid = atoi(json_item_humidity->valuestring);
            strcpy(weather_info[location]->describe, json_item_text->valuestring);
        } else {
            ESP_LOGE(TAG, "Error parsing object - [%s] - [%d]", __FILE__, __LINE__);
            return ESP_FAIL;
        }
        cJSON_Delete(json);
    } else {
        ESP_LOGE(TAG, "Error parsing object - [%s] - [%d]", __FILE__, __LINE__);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t app_weather_parse_air(char *buffer, location_num_t location)
{
    cJSON *json = cJSON_Parse(buffer);
    cJSON *json_air = NULL;

    if (NULL != json) {
        json_air = cJSON_GetObjectItem(json, "now");

        if (NULL != json_air) {
            cJSON *json_item_category = cJSON_GetObjectItem(json_air, "category");
            cJSON *json_item_pm2p5 = cJSON_GetObjectItem(json_air, "pm2p5");
            cJSON *json_item_aqi = cJSON_GetObjectItem(json_air, "aqi");
            cJSON *json_item_pm10 = cJSON_GetObjectItem(json_air, "pm10");
            cJSON *json_item_no2 = cJSON_GetObjectItem(json_air, "no2");
            cJSON *json_item_so2 = cJSON_GetObjectItem(json_air, "so2");
            cJSON *json_item_co = cJSON_GetObjectItem(json_air, "co");
            cJSON *json_item_o3 = cJSON_GetObjectItem(json_air, "o3");

            LOG_TRACE("aqi  : %s", json_item_aqi->valuestring);
            LOG_TRACE("level: %s", json_item_category->valuestring);
            LOG_TRACE("pm10 : %s", json_item_pm10->valuestring);
            LOG_TRACE("pm2.5 : %s", json_item_pm2p5->valuestring);
            LOG_TRACE("no2  : %s", json_item_no2->valuestring);
            LOG_TRACE("so2  : %s", json_item_so2->valuestring);
            LOG_TRACE("co   : %s", json_item_co->valuestring);
            LOG_TRACE("o3   : %s", json_item_o3->valuestring);

            strcpy(air_info->aqi, json_item_aqi->valuestring);
            strcpy(air_info->level, json_item_category->valuestring);
            strcpy(air_info->co, json_item_co->valuestring);
            strcpy(air_info->no2, json_item_no2->valuestring);
            strcpy(air_info->o3, json_item_o3->valuestring);
            strcpy(air_info->pm2p5, json_item_pm2p5->valuestring);
            strcpy(air_info->pm10, json_item_pm10->valuestring);
            strcpy(air_info->so2, json_item_so2->valuestring);
        } else {
            ESP_LOGE(TAG, "Error parsing object - [%s] - [%d]", __FILE__, __LINE__);
            return ESP_FAIL;
        }

        /* Delete object */
        cJSON_Delete(json);
    } else {
        ESP_LOGE(TAG, "Error parsing object - [%s] - [%d]", __FILE__, __LINE__);
        return ESP_FAIL;
    }


    return ESP_OK;
}

esp_err_t app_weather_get_current_info(weather_info_t *info, location_num_t location)
{
    CHECK(info, "Parsing NULL of info", ESP_ERR_INVALID_ARG);
    if (weather_info[location]) {
        memcpy(info, weather_info[location], sizeof(weather_info_t));
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t app_weather_get_air_info(air_info_t *info)
{
    if (NULL == air_info) {
        air_info = heap_caps_malloc(sizeof(air_info_t), MALLOC_CAP_SPIRAM);
        CHECK(air_info, "Failed allocate mem", ESP_ERR_NO_MEM);

        air_info->aqi = heap_caps_malloc(8, MALLOC_CAP_SPIRAM);
        air_info->level = heap_caps_malloc(16, MALLOC_CAP_SPIRAM);
        air_info->co = heap_caps_malloc(8, MALLOC_CAP_SPIRAM);
        air_info->no2 = heap_caps_malloc(8, MALLOC_CAP_SPIRAM);
        air_info->o3 = heap_caps_malloc(8, MALLOC_CAP_SPIRAM);
        air_info->pm2p5 = heap_caps_malloc(8, MALLOC_CAP_SPIRAM);
        air_info->pm10 = heap_caps_malloc(8, MALLOC_CAP_SPIRAM);
        air_info->so2 = heap_caps_malloc(8, MALLOC_CAP_SPIRAM);

        CHECK(air_info->aqi, "Failed allocate mem", ESP_ERR_NO_MEM);
        CHECK(air_info->level, "Failed allocate mem", ESP_ERR_NO_MEM);
        CHECK(air_info->co, "Failed allocate mem", ESP_ERR_NO_MEM);
        CHECK(air_info->no2, "Failed allocate mem", ESP_ERR_NO_MEM);
        CHECK(air_info->o3, "Failed allocate mem", ESP_ERR_NO_MEM);
        CHECK(air_info->pm2p5, "Failed allocate mem", ESP_ERR_NO_MEM);
        CHECK(air_info->pm10, "Failed allocate mem", ESP_ERR_NO_MEM);
        CHECK(air_info->so2, "Failed allocate mem", ESP_ERR_NO_MEM);

        strcpy(air_info->aqi, "18");
        strcpy(air_info->level, "Excellent");
        strcpy(air_info->co, "0.5");
        strcpy(air_info->no2, "12");
        strcpy(air_info->o3, "101");
        strcpy(air_info->pm2p5, "5");
        strcpy(air_info->pm10, "15");
        strcpy(air_info->so2, "3");
    }

    CHECK(info, "Parsing NULL of info", ESP_ERR_INVALID_ARG);

    memcpy(info, air_info, sizeof(air_info_t));

    return ESP_OK;
}
