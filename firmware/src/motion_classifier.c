#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "motion_classifier.h"

#define WINDOW_SIZE 50
#define STEP_SIZE 25

static float accel_buffer[WINDOW_SIZE][3];
static int buffer_head;
static int buffer_count;
static int frame_counter;
static motion_type_t current_motion;

esp_err_t motion_classifier_init(void)
{
    memset(accel_buffer, 0, sizeof(accel_buffer));
    buffer_head = 0;
    buffer_count = 0;
    frame_counter = 0;
    current_motion = MOTION_STATIC;
    return ESP_OK;
}

esp_err_t motion_classifier_feed_data(float ax, float ay, float az, float gx, float gy, float gz)
{
    int idx = buffer_head % WINDOW_SIZE;
    accel_buffer[idx][0] = ax;
    accel_buffer[idx][1] = ay;
    accel_buffer[idx][2] = az;
    buffer_head++;
    if (buffer_count < WINDOW_SIZE) buffer_count++;
    frame_counter++;
    return ESP_OK;
}

static void compute_features(float *variance, float *dyn_range, int *zero_crossings)
{
    float mean_x = 0.0f, mean_y = 0.0f, mean_z = 0.0f;
    float min_mag = 1e10f, max_mag = 0.0f;
    float prev_mag = 0.0f;
    int zc = 0;

    for (int i = 0; i < WINDOW_SIZE; i++) {
        int idx = (buffer_head - WINDOW_SIZE + i) % WINDOW_SIZE;
        mean_x += accel_buffer[idx][0];
        mean_y += accel_buffer[idx][1];
        mean_z += accel_buffer[idx][2];
    }
    mean_x /= WINDOW_SIZE;
    mean_y /= WINDOW_SIZE;
    mean_z /= WINDOW_SIZE;

    float var = 0.0f;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        int idx = (buffer_head - WINDOW_SIZE + i) % WINDOW_SIZE;
        float dx = accel_buffer[idx][0] - mean_x;
        float dy = accel_buffer[idx][1] - mean_y;
        float dz = accel_buffer[idx][2] - mean_z;
        var += dx * dx + dy * dy + dz * dz;

        float mag = sqrtf(
            accel_buffer[idx][0] * accel_buffer[idx][0] +
            accel_buffer[idx][1] * accel_buffer[idx][1] +
            accel_buffer[idx][2] * accel_buffer[idx][2]
        );
        if (mag < min_mag) min_mag = mag;
        if (mag > max_mag) max_mag = mag;

        if (i > 0) {
            if ((prev_mag - 9.8f) * (mag - 9.8f) < 0) zc++;
        }
        prev_mag = mag;
    }
    *variance = var / WINDOW_SIZE;
    *dyn_range = max_mag - min_mag;
    *zero_crossings = zc;
}

int motion_classifier_process(void)
{
    if (buffer_count < WINDOW_SIZE) {
        current_motion = MOTION_STATIC;
        return current_motion;
    }

    if (frame_counter % STEP_SIZE != 0) {
        return current_motion;
    }

    float variance, dyn_range;
    int zero_crossings;
    compute_features(&variance, &dyn_range, &zero_crossings);

    if (variance < 0.5f && dyn_range < 1.0f) {
        current_motion = MOTION_STATIC;
    } else if (variance > 0.3f && variance < 2.0f &&
               dyn_range > 1.0f && dyn_range < 4.0f &&
               zero_crossings >= 5 && zero_crossings <= 20) {
        current_motion = MOTION_WALKING;
    } else if (variance > 2.0f && dyn_range > 4.0f && zero_crossings > 15) {
        current_motion = MOTION_RUNNING;
    } else {
        current_motion = MOTION_CYCLING;
    }

    return current_motion;
}

motion_type_t motion_classifier_get_result(void)
{
    return current_motion;
}

uint8_t motion_classifier_get_confidence(void)
{
    if (buffer_count < WINDOW_SIZE) return 0;

    float variance, dyn_range;
    int zero_crossings;
    compute_features(&variance, &dyn_range, &zero_crossings);

    if (variance < 0.5f && dyn_range < 1.0f) {
        return 90;
    } else if (variance > 2.0f && dyn_range > 4.0f && zero_crossings > 15) {
        return 85;
    } else if (variance > 0.3f && zero_crossings >= 5) {
        return 75;
    }
    return 60;
}
