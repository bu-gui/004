#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "arrhythmia_screener.h"

#define RR_BUFFER_SIZE 60

static float rr_buffer[RR_BUFFER_SIZE];
static int rr_count;
static int rr_head;

esp_err_t arrhythmia_screener_init(void)
{
    memset(rr_buffer, 0, sizeof(rr_buffer));
    rr_count = 0;
    rr_head = 0;
    return ESP_OK;
}

esp_err_t arrhythmia_screener_feed_rr_interval(uint32_t rr_ms)
{
    rr_buffer[rr_head] = (float)rr_ms;
    rr_head = (rr_head + 1) % RR_BUFFER_SIZE;
    if (rr_count < RR_BUFFER_SIZE) rr_count++;
    return ESP_OK;
}

uint32_t arrhythmia_screener_get_sdnn(void)
{
    if (rr_count < 2) return 0;

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
    return (uint32_t)sqrtf(variance / n);
}

arrhythmia_result_t arrhythmia_screener_analyze(void)
{
    if (rr_count < 2) return ARRHYTHMIA_INSUFFICIENT_DATA;

    int n = rr_count < RR_BUFFER_SIZE ? rr_count : RR_BUFFER_SIZE;

    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        sum += rr_buffer[i];
    }
    float avg_rr = sum / n;

    float variance = 0.0f;
    for (int i = 0; i < n; i++) {
        float diff = rr_buffer[i] - avg_rr;
        variance += diff * diff;
    }
    float sdnn = sqrtf(variance / n);

    float sum_diff_sq = 0.0f;
    int nn50 = 0;
    for (int i = 1; i < n; i++) {
        float diff = rr_buffer[i] - rr_buffer[i - 1];
        sum_diff_sq += diff * diff;
        if (fabsf(diff) > 50.0f) nn50++;
    }
    float rmssd = sqrtf(sum_diff_sq / (n - 1));
    float pnn50 = (float)nn50 / (n - 1) * 100.0f;

    if (sdnn > 50.0f && pnn50 > 30.0f && rmssd > 40.0f) {
        return ARRHYTHMIA_SUSPECT_AF;
    }

    int short_rr_count = 0;
    float threshold_short = avg_rr * 0.6f;
    for (int i = 0; i < n; i++) {
        if (rr_buffer[i] < threshold_short) short_rr_count++;
    }
    if (short_rr_count >= 1 && short_rr_count <= 3 && sdnn > 30.0f) {
        return ARRHYTHMIA_SUSPECT_PVC;
    }

    return ARRHYTHMIA_NORMAL;
}

esp_err_t arrhythmia_screener_reset(void)
{
    arrhythmia_screener_init();
    return ESP_OK;
}
