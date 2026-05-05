#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef enum {
    POWER_ACTIVE      = 0,
    POWER_MONITORING  = 1,
    POWER_DAILY       = 2,
    POWER_LIGHT_SLEEP = 3,
    POWER_DEEP_SLEEP  = 4
} power_mode_t;

esp_err_t power_mgmt_init(void);
esp_err_t power_mgmt_set_mode(power_mode_t mode);
power_mode_t power_mgmt_get_mode(void);
float power_mgmt_get_battery_voltage(void);
uint8_t power_mgmt_get_battery_percent(void);
esp_err_t power_mgmt_update_screen_power(bool on);
esp_err_t power_mgmt_enter_deep_sleep(void);
