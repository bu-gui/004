#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef struct {
    float bpm;
    bool data_ready;
    uint8_t confidence;
} heart_rate_result_t;

esp_err_t heart_rate_init(void);
esp_err_t heart_rate_feed_sample(uint32_t ir_raw, uint32_t red_raw);
esp_err_t heart_rate_get_result(heart_rate_result_t *result);
esp_err_t heart_rate_reset(void);
