#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef struct {
    float sleep_hours;
    uint8_t quality;
    uint8_t deep_pct;
    char fall_asleep_time[6];
    char wake_up_time[6];
} sleep_result_t;

esp_err_t sleep_analyzer_init(void);
esp_err_t sleep_analyzer_feed_data(float heart_rate, uint8_t spo2, float ax, float ay, float az);
sleep_result_t sleep_analyzer_analyze(void);
esp_err_t sleep_analyzer_get_result(sleep_result_t *result);
