#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "wifi_conn.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_CONNECT_TIMEOUT_MS 10000

static const char *TAG = "WIFI_CONN";

static EventGroupHandle_t wifi_event_group;
static bool wifi_connected;
static wifi_conn_callback_t conn_callback;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        if (conn_callback) conn_callback(false);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        if (conn_callback) conn_callback(true);
    }
}

esp_err_t wifi_conn_init(void)
{
    wifi_connected = false;

    wifi_event_group = xEventGroupCreate();

    esp_err_t ret;
    ret = esp_netif_init();
    if (ret != ESP_OK) return ret;
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) return ret;
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) return ret;

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler, NULL, &instance_got_ip);

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) return ret;
    ret = esp_wifi_start();
    if (ret != ESP_OK) return ret;

    return ESP_OK;
}

esp_err_t wifi_conn_connect(void)
{
    wifi_config_t wifi_config = {
        .sta = {
            .threshold = {
                .authmode = WIFI_AUTH_WPA2_PSK,
            },
        },
    };

    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) return ret;

    ret = esp_wifi_start();
    if (ret != ESP_OK) return ret;

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                                           pdFALSE, pdTRUE,
                                           pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to WiFi");
        return ESP_OK;
    }

    ESP_LOGW(TAG, "WiFi connection timeout");
    return ESP_ERR_TIMEOUT;
}

esp_err_t wifi_conn_disconnect(void)
{
    esp_err_t ret = esp_wifi_disconnect();
    wifi_connected = false;
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    return ret;
}

bool wifi_conn_is_connected(void)
{
    return wifi_connected;
}

esp_err_t wifi_conn_register_callback(wifi_conn_callback_t callback)
{
    conn_callback = callback;
    return ESP_OK;
}
