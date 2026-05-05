#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef struct {
    uint32_t total_steps;
    uint32_t cadence;
    float distance_km;
} step_counter_t;

esp_err_t step_counter_init(void);
esp_err_t step_counter_feed_accel(float ax, float ay, float az);
esp_err_t step_counter_get_result(step_counter_t *result);
esp_err_t step_counter_reset(void);
