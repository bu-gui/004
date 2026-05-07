#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef enum {
    FALL_NONE      = 0,
    FALL_DETECTED  = 1,
    FALL_CONFIRMED = 2
} fall_state_t;

esp_err_t fall_detector_init(void);
esp_err_t fall_detector_feed_accel(float ax, float ay, float az);
esp_err_t fall_detector_process(void);
fall_state_t fall_detector_get_state(void);
bool fall_detector_is_falling(void);
esp_err_t fall_detector_acknowledge(void);
