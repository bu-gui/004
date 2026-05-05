#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RR_BUFFER_SIZE 60

typedef enum {
    NORMAL = 0,
    SUSPECT_AF,
    SUSPECT_PVC
} arrhythmia_type_t;

typedef struct {
    arrhythmia_type_t type;
    float sdnn;
    float rmssd;
    float pnn50;
    float avg_rr;
} arrhythmia_result_t;

static float rr_buffer[RR_BUFFER_SIZE];
static int rr_count;
static int rr_head;

void arrhythmia_screener_init(void)
{
    memset(rr_buffer, 0, sizeof(rr_buffer));
    rr_count = 0;
    rr_head = 0;
}

void feed_rr_interval(float rr_ms)
{
    rr_buffer[rr_head] = rr_ms;
    rr_head = (rr_head + 1) % RR_BUFFER_SIZE;
    if (rr_count < RR_BUFFER_SIZE) rr_count++;
}

float get_sdnn(void)
{
    if (rr_count < 2) return 0.0f;

    float sum = 0.0f;
    int n = rr_count < RR_BUFFER_SIZE ? rr_count : RR_BUFFER_SIZE;
    for (int i = 0; i < n; i++) {
        sum += rr_buffer[i];
    }
    float mean = sum / n;

    float variance = 0.0f;
    for (int i = 0; i < n; i++) {
        float diff = rr_buffer[i] - mean;
        variance += diff * diff;
    }
    return sqrtf(variance / n);
}

arrhythmia_result_t arrhythmia_screener_analyze(void)
{
    arrhythmia_result_t res;
    memset(&res, 0, sizeof(res));
    res.type = NORMAL;

    if (rr_count < 2) return res;

    int n = rr_count < RR_BUFFER_SIZE ? rr_count : RR_BUFFER_SIZE;

    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        sum += rr_buffer[i];
    }
    res.avg_rr = sum / n;

    float variance = 0.0f;
    for (int i = 0; i < n; i++) {
        float diff = rr_buffer[i] - res.avg_rr;
        variance += diff * diff;
    }
    res.sdnn = sqrtf(variance / n);

    float sum_diff_sq = 0.0f;
    int nn50 = 0;
    for (int i = 1; i < n; i++) {
        float diff = rr_buffer[i] - rr_buffer[i - 1];
        sum_diff_sq += diff * diff;
        if (fabsf(diff) > 50.0f) nn50++;
    }
    res.rmssd = sqrtf(sum_diff_sq / (n - 1));
    res.pnn50 = (float)nn50 / (n - 1) * 100.0f;

    if (res.sdnn > 50.0f && res.pnn50 > 30.0f && res.rmssd > 40.0f) {
        res.type = SUSPECT_AF;
        return res;
    }

    int short_rr_count = 0;
    float threshold_short = res.avg_rr * 0.6f;
    for (int i = 0; i < n; i++) {
        if (rr_buffer[i] < threshold_short) short_rr_count++;
    }
    if (short_rr_count >= 1 && short_rr_count <= 3 && res.sdnn > 30.0f) {
        res.type = SUSPECT_PVC;
        return res;
    }

    res.type = NORMAL;
    return res;
}
