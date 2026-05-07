#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include "calorie.h"

esp_err_t motion_classifier_init(void);
esp_err_t motion_classifier_feed_data(float ax, float ay, float az, float gx, float gy, float gz);
motion_type_t motion_classifier_get_result(void);
uint8_t motion_classifier_get_confidence(void);
int motion_classifier_process(void);
