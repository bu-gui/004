#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "cJSON.h"
#include "deepseek_api.h"

#define DEEPSEEK_API_URL "https://api.deepseek.com/v1/chat/completions"
#ifdef CONFIG_DEEPSEEK_API_KEY
#define DEEPSEEK_API_KEY CONFIG_DEEPSEEK_API_KEY
#else
#define DEEPSEEK_API_KEY ""
#endif
#define MAX_RESPONSE_SIZE 4096

static const char *TAG = "deepseek_api";

static bool api_ready;

static char response_buffer[MAX_RESPONSE_SIZE];

static char *http_post_json(const char *json_payload);

esp_err_t deepseek_api_init(void)
{
    api_ready = true;
    return ESP_OK;
}

esp_err_t deepseek_api_upload_data(const daily_report_t *report)
{
    if (!api_ready) return ESP_ERR_INVALID_STATE;
    if (!report) return ESP_ERR_INVALID_ARG;

    cJSON *root = cJSON_CreateObject();
    cJSON *messages = cJSON_AddArrayToObject(root, "messages");

    cJSON *sys_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(sys_msg, "role", "system");
    cJSON_AddStringToObject(sys_msg, "content", "You are a health data analyzer. Analyze the provided daily report data.");
    cJSON_AddItemToArray(messages, sys_msg);

    cJSON *user_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(user_msg, "role", "user");

    cJSON *data_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(data_obj, "date", report->date);
    cJSON_AddNumberToObject(data_obj, "steps", report->total_steps);
    cJSON_AddNumberToObject(data_obj, "calories", report->total_calories);
    cJSON_AddNumberToObject(data_obj, "avg_heart_rate", report->avg_heart_rate);
    cJSON_AddNumberToObject(data_obj, "min_heart_rate", report->min_heart_rate);
    cJSON_AddNumberToObject(data_obj, "max_heart_rate", report->max_heart_rate);
    cJSON_AddNumberToObject(data_obj, "avg_spo2", report->avg_spo2);
    cJSON_AddNumberToObject(data_obj, "sleep_hours", report->sleep_hours);
    cJSON_AddNumberToObject(data_obj, "sleep_quality", report->sleep_quality);
    cJSON_AddNumberToObject(data_obj, "fall_count", report->fall_count);

    char *data_str = cJSON_PrintUnformatted(data_obj);
    cJSON_AddStringToObject(user_msg, "content", data_str);
    free(data_str);
    cJSON_Delete(data_obj);

    cJSON_AddItemToArray(messages, user_msg);

    cJSON_AddStringToObject(root, "model", "deepseek-chat");
    cJSON_AddNumberToObject(root, "max_tokens", 512);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    char *result = http_post_json(payload);
    free(payload);

    if (result) {
        free(result);
        return ESP_OK;
    }

    return ESP_FAIL;
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (evt->data && evt->data_len < MAX_RESPONSE_SIZE) {
                strncat(response_buffer, (char *)evt->data, evt->data_len);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            break;
        default:
            break;
    }
    return ESP_OK;
}

static char *http_post_json(const char *json_payload)
{
    memset(response_buffer, 0, sizeof(response_buffer));

    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        ESP_LOGE(TAG, "WiFi not connected");
        return NULL;
    }

    esp_http_client_config_t config = {
        .url = DEEPSEEK_API_URL,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .timeout_ms = 30000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", "Bearer " DEEPSEEK_API_KEY);

    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));
    esp_err_t err = esp_http_client_perform(client);

    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        return NULL;
    }

    esp_http_client_cleanup(client);

    if (strlen(response_buffer) == 0) return NULL;

    cJSON *root = cJSON_Parse(response_buffer);
    if (!root) return NULL;

    cJSON *choices = cJSON_GetObjectItem(root, "choices");
    if (!choices || !cJSON_IsArray(choices)) {
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *first = cJSON_GetArrayItem(choices, 0);
    if (!first) {
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *message = cJSON_GetObjectItem(first, "message");
    if (!message) {
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *content = cJSON_GetObjectItem(message, "content");
    if (!content || !cJSON_IsString(content)) {
        cJSON_Delete(root);
        return NULL;
    }

    char *result = strdup(content->valuestring);
    cJSON_Delete(root);
    return result;
}


