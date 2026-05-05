#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SAMPLE_RATE 100
#define BUFFER_SIZE 500
#define DC_ALPHA 0.9f
#define BANDPASS_B0 0.1316f
#define BANDPASS_A1 1.4793f
#define BANDPASS_A2 0.6307f
#define THRESHOLD_WINDOW 50
#define THRESHOLD_RATIO 0.6f
#define MIN_PEAK_INTERVAL_MS 300
#define MIN_BPM 30
#define MAX_BPM 220

typedef struct {
    float buffer[BUFFER_SIZE];
    int head;
    int count;
} ring_buffer_t;

typedef struct {
    float bpm;
    float confidence;
} heart_rate_result_t;

static ring_buffer_t hr_buffer;
static float dc_prev;
static float bp_x1, bp_x2, bp_y1, bp_y2;
static float last_peak_value;
static int last_peak_index;
static float rr_intervals[10];
static int rr_count;
static int sample_index;

void heart_rate_init(void)
{
    memset(&hr_buffer, 0, sizeof(ring_buffer_t));
    dc_prev = 0.0f;
    bp_x1 = bp_x2 = bp_y1 = bp_y2 = 0.0f;
    last_peak_value = 0.0f;
    last_peak_index = -MIN_PEAK_INTERVAL_MS;
    rr_count = 0;
    sample_index = 0;
    memset(rr_intervals, 0, sizeof(rr_intervals));
}

static float update_dc_filter(float raw)
{
    dc_prev = DC_ALPHA * dc_prev + (1.0f - DC_ALPHA) * raw;
    return raw - dc_prev;
}

static float update_bandpass(float ac)
{
    float y = BANDPASS_B0 * ac + BANDPASS_B0 * bp_x1 + BANDPASS_B0 * bp_x2
              + BANDPASS_A1 * bp_y1 - BANDPASS_A2 * bp_y2;
    bp_x2 = bp_x1;
    bp_x1 = ac;
    bp_y2 = bp_y1;
    bp_y1 = y;
    return y;
}

static void ring_push(ring_buffer_t *rb, float val)
{
    rb->buffer[rb->head] = val;
    rb->head = (rb->head + 1) % BUFFER_SIZE;
    if (rb->count < BUFFER_SIZE) rb->count++;
}

static float ring_get(ring_buffer_t *rb, int offset)
{
    int idx = (rb->head - offset - 1 + BUFFER_SIZE) % BUFFER_SIZE;
    return rb->buffer[idx];
}

void heart_rate_feed_sample(uint32_t ir_value)
{
    float raw = (float)ir_value;
    float ac = update_dc_filter(raw);
    float filtered = update_bandpass(ac);

    ring_push(&hr_buffer, filtered);
    sample_index++;

    int valid_count = hr_buffer.count < THRESHOLD_WINDOW ? hr_buffer.count : THRESHOLD_WINDOW;
    float max_val = 0.0f;
    for (int i = 0; i < valid_count; i++) {
        float v = fabsf(ring_get(&hr_buffer, i));
        if (v > max_val) max_val = v;
    }
    float threshold = max_val * THRESHOLD_RATIO;
    if (threshold < 0.001f) threshold = 0.001f;

    if (filtered > threshold && filtered > last_peak_value) {
        last_peak_value = filtered;
    }

    if (filtered < threshold * 0.5f && last_peak_value > threshold) {
        int elapsed_ms = (sample_index - last_peak_index) * 1000 / SAMPLE_RATE;
        if (elapsed_ms >= MIN_PEAK_INTERVAL_MS) {
            float rr_ms = (float)elapsed_ms;
            if (rr_count < 10) {
                rr_intervals[rr_count++] = rr_ms;
            } else {
                for (int i = 0; i < 9; i++) rr_intervals[i] = rr_intervals[i + 1];
                rr_intervals[9] = rr_ms;
            }
            last_peak_index = sample_index;
        }
        last_peak_value = 0.0f;
    }
}

heart_rate_result_t heart_rate_get_result(void)
{
    heart_rate_result_t res = {0, 0.0f};
    if (rr_count < 2) return res;

    float sum = 0.0f;
    for (int i = 0; i < rr_count; i++) {
        sum += rr_intervals[i];
    }
    float avg_interval = sum / rr_count;
    if (avg_interval < 1.0f) return res;

    float bpm = 60000.0f / avg_interval;
    if (bpm < MIN_BPM) bpm = MIN_BPM;
    if (bpm > MAX_BPM) bpm = MAX_BPM;
    res.bpm = bpm;

    float variance = 0.0f;
    for (int i = 0; i < rr_count; i++) {
        float diff = rr_intervals[i] - avg_interval;
        variance += diff * diff;
    }
    variance /= rr_count;
    float cv = sqrtf(variance) / avg_interval;
    res.confidence = 1.0f - (cv < 0.5f ? cv : 0.5f) * 2.0f;
    if (res.confidence < 0.0f) res.confidence = 0.0f;

    return res;
}
