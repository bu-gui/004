#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BODY_MOVEMENT_WINDOW 600

typedef enum {
    AWAKE = 0,
    LIGHT_SLEEP,
    DEEP_SLEEP
} sleep_stage_t;

typedef struct {
    sleep_stage_t stage;
    int total_minutes;
    int deep_sleep_minutes;
    int light_sleep_minutes;
    float quality_score;
} sleep_result_t;

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

void sleep_analyzer_init(void)
{
    memset(movement_buffer, 0, sizeof(movement_buffer));
    movement_head = 0;
    movement_count = 0;
    prev_ax = prev_ay = prev_az = 0.0f;
    sleep_started = 0;
    memset(&result, 0, sizeof(sleep_result_t));
    result.stage = AWAKE;
}

void feed_data(float ax, float ay, float az, float hr)
{
    int idx = movement_head % BODY_MOVEMENT_WINDOW;
    movement_buffer[idx].ax = ax;
    movement_buffer[idx].ay = ay;
    movement_buffer[idx].az = az;
    movement_buffer[idx].hr = hr;
    movement_head++;
    if (movement_count < BODY_MOVEMENT_WINDOW) movement_count++;

    prev_ax = ax;
    prev_ay = ay;
    prev_az = az;
}

sleep_result_t sleep_analyzer_analyze(void)
{
    if (movement_count < 60) {
        result.stage = AWAKE;
        result.quality_score = 0.0f;
        return result;
    }

    int n = movement_count < BODY_MOVEMENT_WINDOW ? movement_count : BODY_MOVEMENT_WINDOW;
    int start = movement_head - n;

    float movement_sum = 0.0f;
    float hr_sum = 0.0f;
    float hr_var_sum = 0.0f;
    float prev_hr = 0.0f;
    int hr_samples = 0;

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
                hr_var_sum += (movement_buffer[idx].hr - prev_hr) * (movement_buffer[idx].hr - prev_hr);
            }
            prev_hr = movement_buffer[idx].hr;
            hr_samples++;
        }
    }

    float avg_movement = movement_sum / n;
    float avg_hr = hr_samples > 0 ? hr_sum / hr_samples : 0.0f;
    float hrv = hr_samples > 1 ? sqrtf(hr_var_sum / hr_samples) : 0.0f;

    if (!sleep_started) {
        if (avg_movement < 0.3f && avg_hr > 0 && avg_hr < 65.0f) {
            sleep_started = 1;
            result.stage = LIGHT_SLEEP;
            result.total_minutes = 0;
        }
    }

    if (sleep_started) {
        result.total_minutes++;

        if (avg_movement < 0.15f && hrv > 30.0f) {
            result.stage = DEEP_SLEEP;
            result.deep_sleep_minutes++;
        } else if (avg_movement < 0.5f) {
            result.stage = LIGHT_SLEEP;
            result.light_sleep_minutes++;
        } else {
            result.stage = AWAKE;
            if (result.total_minutes > 5) {
                sleep_started = 0;
                result.total_minutes = 0;
                result.deep_sleep_minutes = 0;
                result.light_sleep_minutes = 0;
            }
        }
    }

    if (result.total_minutes > 0) {
        float duration_score = (result.total_minutes < 480)
            ? (float)result.total_minutes / 480.0f * 40.0f
            : 40.0f;

        float deep_ratio = result.total_minutes > 0
            ? (float)result.deep_sleep_minutes / result.total_minutes
            : 0.0f;
        float deep_score = deep_ratio * 30.0f;

        float movement_score = (1.0f - (avg_movement < 1.0f ? avg_movement : 1.0f)) * 20.0f;

        float hrv_score = (hrv < 50.0f ? hrv / 50.0f : 1.0f) * 10.0f;

        result.quality_score = duration_score + deep_score + movement_score + hrv_score;
        if (result.quality_score > 100.0f) result.quality_score = 100.0f;
    }

    return result;
}
