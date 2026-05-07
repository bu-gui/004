#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sleep_analyzer.h"

#define BODY_MOVEMENT_WINDOW 600

typedef struct {
    float ax, ay, az;
    float hr;
} sensor_data_t;

static sensor_data_t movement_buffer[BODY_MOVEMENT_WINDOW];
static int movement_head;
static int movement_count;
static float prev_ax, prev_ay, prev_az;
static int sleep_started;
static sleep_result_t result;

esp_err_t sleep_analyzer_init(void)
{
    memset(movement_buffer, 0, sizeof(movement_buffer));
    movement_head = 0;
    movement_count = 0;
    prev_ax = prev_ay = prev_az = 0.0f;
    sleep_started = 0;
    memset(&result, 0, sizeof(sleep_result_t));
    result.sleep_hours = 0.0f;
    result.quality = 0;
    result.deep_pct = 0;
    return ESP_OK;
}

esp_err_t sleep_analyzer_feed_data(float heart_rate, uint8_t spo2, float ax, float ay, float az)
{
    (void)spo2;
    int idx = movement_head % BODY_MOVEMENT_WINDOW;
    movement_buffer[idx].ax = ax;
    movement_buffer[idx].ay = ay;
    movement_buffer[idx].az = az;
    movement_buffer[idx].hr = heart_rate;
    movement_head++;
    if (movement_count < BODY_MOVEMENT_WINDOW) movement_count++;

    prev_ax = ax;
    prev_ay = ay;
    prev_az = az;
    return ESP_OK;
}

sleep_result_t sleep_analyzer_analyze(void)
{
    sleep_result_t res;
    memset(&res, 0, sizeof(res));

    if (movement_count < 60) {
        res.sleep_hours = 0.0f;
        res.quality = 0;
        res.deep_pct = 0;
        return res;
    }

    int n = movement_count < BODY_MOVEMENT_WINDOW ? movement_count : BODY_MOVEMENT_WINDOW;
    int start = movement_head - n;

    float movement_sum = 0.0f;
    float hr_sum = 0.0f;
    float hr_diff_sq_sum = 0.0f;
    float prev_hr = 0.0f;
    int hr_samples = 0;
    int total_deep_samples = 0;

    for (int i = 0; i < n; i++) {
        int idx = (start + i + BODY_MOVEMENT_WINDOW) % BODY_MOVEMENT_WINDOW;
        float mag = sqrtf(
            movement_buffer[idx].ax * movement_buffer[idx].ax +
            movement_buffer[idx].ay * movement_buffer[idx].ay +
            movement_buffer[idx].az * movement_buffer[idx].az
        );
        movement_sum += fabsf(mag - 9.8f);

        if (movement_buffer[idx].hr > 0) {
            hr_sum += movement_buffer[idx].hr;
            if (hr_samples > 0) {
                float diff = movement_buffer[idx].hr - prev_hr;
                hr_diff_sq_sum += diff * diff;
            }
            prev_hr = movement_buffer[idx].hr;
            hr_samples++;
        }
    }

    float avg_movement = movement_sum / n;
    float avg_hr = hr_samples > 0 ? hr_sum / hr_samples : 0.0f;
    float hrv_rmssd = hr_samples > 1 ? sqrtf(hr_diff_sq_sum / (hr_samples - 1)) : 0.0f;

    if (!sleep_started) {
        if (avg_movement < 0.3f && avg_hr > 0 && avg_hr < 65.0f) {
            sleep_started = 1;
            result.sleep_hours = 0.0f;
            result.quality = 0;
            result.deep_pct = 0;
        }
    }

    if (sleep_started) {
        result.sleep_hours += 1.0f / 60.0f;

        if (avg_movement < 0.15f && hrv_rmssd > 30.0f) {
            total_deep_samples++;
        }

        if (avg_movement >= 0.5f && result.sleep_hours > 0.1f) {
            sleep_started = 0;
        }
    }

    if (result.sleep_hours > 0) {
        float deep_ratio = n > 0 ? (float)total_deep_samples / n : 0.0f;
        result.deep_pct = (uint8_t)(deep_ratio * 100.0f);

        float duration_score = (result.sleep_hours < 8.0f)
            ? result.sleep_hours / 8.0f * 40.0f
            : 40.0f;

        float deep_score = deep_ratio * 30.0f;

        float movement_score = (1.0f - (avg_movement < 1.0f ? avg_movement : 1.0f)) * 20.0f;

        float hrv_score = (hrv_rmssd < 50.0f ? hrv_rmssd / 50.0f : 1.0f) * 10.0f;

        float quality_score = duration_score + deep_score + movement_score + hrv_score;
        if (quality_score > 100.0f) quality_score = 100.0f;
        result.quality = (uint8_t)quality_score;
    }

    memcpy(&res, &result, sizeof(sleep_result_t));
    return res;
}

esp_err_t sleep_analyzer_get_result(sleep_result_t *result_out)
{
    if (!result_out) return ESP_ERR_INVALID_ARG;
    memcpy(result_out, &result, sizeof(sleep_result_t));
    return ESP_OK;
}
