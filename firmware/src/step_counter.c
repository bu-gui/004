#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "step_counter.h"

#define WINDOW_SIZE 20
#define THRESHOLD 2.5f
#define MIN_INTERVAL_MS 200

typedef struct {
    float magnitude[WINDOW_SIZE];
    int head;
    int count;
} accel_ring_t;

static accel_ring_t accel_ring;
static int prev_above_threshold;
static int last_step_time;
static int sample_index;
static int above_count;
static step_counter_t result;

esp_err_t step_counter_init(void)
{
    memset(&accel_ring, 0, sizeof(accel_ring_t));
    memset(&result, 0, sizeof(step_counter_t));
    prev_above_threshold = 0;
    last_step_time = 0;
    sample_index = 0;
    above_count = 0;
    return ESP_OK;
}

static void push_magnitude(float val)
{
    accel_ring.magnitude[accel_ring.head] = val;
    accel_ring.head = (accel_ring.head + 1) % WINDOW_SIZE;
    if (accel_ring.count < WINDOW_SIZE) accel_ring.count++;
}

static float get_magnitude_avg(void)
{
    if (accel_ring.count == 0) return 1.0f;
    float sum = 0.0f;
    int n = accel_ring.count < WINDOW_SIZE ? accel_ring.count : WINDOW_SIZE;
    for (int i = 0; i < n; i++) {
        int idx = (accel_ring.head - i - 1 + WINDOW_SIZE) % WINDOW_SIZE;
        sum += accel_ring.magnitude[idx];
    }
    return sum / n;
}

esp_err_t step_counter_feed_accel(float ax, float ay, float az)
{
    float magnitude = sqrtf(ax * ax + ay * ay + az * az);
    push_magnitude(magnitude);

    float dynamic_mag = fabsf(magnitude - 9.8f);
    int above = (dynamic_mag > THRESHOLD) ? 1 : 0;

    float avg_mag = get_magnitude_avg();
    float avg_dynamic = fabsf(avg_mag - 9.8f);

    if (above && !prev_above_threshold && avg_dynamic > 1.0f) {
        int now = sample_index;
        int elapsed_ms = (now - last_step_time);
        if (elapsed_ms >= MIN_INTERVAL_MS) {
            result.total_steps++;
            if (elapsed_ms > 0) {
                result.cadence = (uint32_t)(60000.0f / elapsed_ms);
            }
            result.distance_km = result.total_steps * 0.7f / 1000.0f;
            last_step_time = now;
        }
    }

    prev_above_threshold = above;
    sample_index++;
    return ESP_OK;
}

esp_err_t step_counter_get_result(step_counter_t *result_out)
{
    if (!result_out) return ESP_ERR_INVALID_ARG;
    result_out->total_steps = result.total_steps;
    result_out->cadence = result.cadence;
    result_out->distance_km = result.distance_km;
    return ESP_OK;
}

esp_err_t step_counter_reset(void)
{
    step_counter_init();
    return ESP_OK;
}
