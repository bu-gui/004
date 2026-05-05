#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef struct {
    uint8_t spo2;
    bool data_ready;
} spo2_result_t;

esp_err_t spo2_init(void);
esp_err_t spo2_feed_sample(uint32_t ir_raw, uint32_t red_raw);
esp_err_t spo2_get_result(spo2_result_t *result);
