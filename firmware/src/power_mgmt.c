#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "power_mgmt.h"

#define BATTERY_ADC_PIN    6

static const char *TAG = "POWER_MGMT";

void enter_deep_sleep(uint32_t seconds)
{
    esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
    esp_deep_sleep_start();
}

esp_err_t power_mgmt_set_mode(power_mode_t mode)
{
    switch (mode) {
        case POWER_ACTIVE:
            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
            ESP_LOGI(TAG, "Power mode: ACTIVE");
            break;

        case POWER_MONITORING:
            ESP_LOGI(TAG, "Power mode: MONITORING");
            break;

        case POWER_DAILY:
            esp_sleep_enable_timer_wakeup(60 * 1000000ULL);
            ESP_LOGI(TAG, "Power mode: DAILY (60s wakeup)");
            break;

        case POWER_LIGHT_SLEEP:
            esp_sleep_enable_timer_wakeup(300 * 1000000ULL);
            ESP_LOGI(TAG, "Power mode: LIGHT_SLEEP (5min wakeup)");
            esp_light_sleep_start();
            break;

        case POWER_DEEP_SLEEP:
            ESP_LOGI(TAG, "Power mode: DEEP_SLEEP");
            enter_deep_sleep(3600);
            break;

        default:
            break;
    }
    return ESP_OK;
}

float power_mgmt_get_battery_voltage(void)
{
    uint32_t adc_reading = 0;
    for (int i = 0; i < 64; i++) {
        adc_reading += adc1_get_raw(ADC1_CHANNEL_5);
    }
    adc_reading /= 64;

    float battery_voltage = (float)adc_reading / 4095.0f * 3.3f * 2.0f;

    return battery_voltage;
}

uint8_t power_mgmt_get_battery_percent(void)
{
    float voltage = power_mgmt_get_battery_voltage();

    if (voltage >= 4.2f) return 100;
    if (voltage >= 4.0f) return (uint8_t)(80.0f + (voltage - 4.0f) / 0.2f * 20.0f);
    if (voltage >= 3.8f) return (uint8_t)(60.0f + (voltage - 3.8f) / 0.2f * 20.0f);
    if (voltage >= 3.6f) return (uint8_t)(40.0f + (voltage - 3.6f) / 0.2f * 20.0f);
    if (voltage >= 3.4f) return (uint8_t)(20.0f + (voltage - 3.4f) / 0.2f * 20.0f);
    if (voltage >= 3.2f) return (uint8_t)(10.0f + (voltage - 3.2f) / 0.2f * 10.0f);
    return 0;
}

esp_err_t power_mgmt_init(void)
{
    gpio_reset_pin(BATTERY_ADC_PIN);
    gpio_set_direction(BATTERY_ADC_PIN, GPIO_MODE_INPUT);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);

    esp_sleep_enable_timer_wakeup(60 * 1000000ULL);

    ESP_LOGI(TAG, "Power management initialized");
    return ESP_OK;
}

esp_err_t power_mgmt_enter_deep_sleep(void)
{
    ESP_LOGI(TAG, "Entering deep sleep");
    esp_deep_sleep_start();
    return ESP_OK;
}
