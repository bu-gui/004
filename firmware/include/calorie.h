#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef enum {
    MOTION_STATIC   = 0,
    MOTION_WALKING  = 1,
    MOTION_RUNNING  = 2,
    MOTION_CYCLING  = 3
} motion_type_t;

esp_err_t calorie_init(void);
esp_err_t calorie_update_steps(uint32_t steps);
esp_err_t calorie_update_heart_rate(float bpm);
esp_err_t calorie_update_motion_type(motion_type_t motion);
float calorie_get_total(void);
esp_err_t calorie_reset(void);
