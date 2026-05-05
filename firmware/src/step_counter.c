#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define WINDOW_SIZE 20
#define THRESHOLD 2.5f
#define MIN_INTERVAL_MS 200

typedef struct {
    float magnitude[WINDOW_SIZE];
    int head;
    int count;
} accel_ring_t;

typedef struct {
    uint32_t total_steps;
    float cadence;
    float distance_meters;
} step_result_t;

static accel_ring_t accel_ring;
static int prev_above_threshold;
static int last_step_time;
static int sample_index;
static int above_count;
static step_result_t result;

void step_counter_init(void)
{
    memset(&accel_ring, 0, sizeof(accel_ring_t));
    memset(&result, 0, sizeof(step_result_t));
    prev_above_threshold = 0;
    last_step_time = 0;
    sample_index = 0;
    above_count = 0;
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

void step_counter_feed_accel(float ax, float ay, float az)
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
                result.cadence = 60000.0f / elapsed_ms;
            }
            result.distance_meters = result.total_steps * 0.7f;
            last_step_time = now;
        }
    }

    prev_above_threshold = above;
    sample_index++;
}

step_result_t step_counter_get_result(void)
{
    return result;
}
