#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef struct {
    uint32_t ir_raw;
    uint32_t red_raw;
    bool data_valid;
} max30102_sample_t;

esp_err_t max30102_init(void);
esp_err_t max30102_read_fifo(max30102_sample_t *sample);
esp_err_t max30102_set_led_current(uint8_t ir_ma, uint8_t red_ma);
esp_err_t max30102_set_sample_rate(uint8_t rate);
esp_err_t max30102_reset(void);
