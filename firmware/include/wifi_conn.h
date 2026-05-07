#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

#define WIFI_SSID "123456789"
#define WIFI_PASS "123456789"

typedef void (*wifi_conn_callback_t)(bool connected);

esp_err_t wifi_conn_init(void);
esp_err_t wifi_conn_connect(void);
esp_err_t wifi_conn_disconnect(void);
bool wifi_conn_is_connected(void);
esp_err_t wifi_conn_register_callback(wifi_conn_callback_t callback);
