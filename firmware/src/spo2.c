#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "spo2.h"

#define BUFFER_SIZE 100
#define HP_ALPHA 0.98f
#define MIN_SPO2 70.0f
#define MAX_SPO2 100.0f
#define SIGNAL_THRESHOLD 100.0f

typedef struct {
    float ir_buffer[BUFFER_SIZE];
    float red_buffer[BUFFER_SIZE];
    int ir_count;
    int red_count;
    float ir_dc;
    float red_dc;
    float ir_hp_prev;
    float red_hp_prev;
    float ir_ac_prev;
    float red_ac_prev;
    uint8_t spo2_value;
    bool data_ready;
    int samples_collected;
} spo2_ctx_t;

static spo2_ctx_t spo2_ctx;

esp_err_t spo2_init(void)
{
    memset(&spo2_ctx, 0, sizeof(spo2_ctx_t));
    spo2_ctx.spo2_value = 95;
    spo2_ctx.data_ready = false;
    return ESP_OK;
}

static float highpass(float input, float *prev, float *ac_prev)
{
    float hp = HP_ALPHA * (*prev) + HP_ALPHA * (input - *ac_prev);
    *prev = hp;
    *ac_prev = input;
    return hp;
}

esp_err_t spo2_feed_sample(uint32_t ir_raw, uint32_t red_raw)
{
    float ir_f = (float)ir_raw;
    float red_f = (float)red_raw;

    spo2_ctx.ir_dc = HP_ALPHA * spo2_ctx.ir_dc + (1.0f - HP_ALPHA) * ir_f;
    spo2_ctx.red_dc = HP_ALPHA * spo2_ctx.red_dc + (1.0f - HP_ALPHA) * red_f;

    float ir_ac = highpass(ir_f, &spo2_ctx.ir_hp_prev, &spo2_ctx.ir_ac_prev);
    float red_ac = highpass(red_f, &spo2_ctx.red_hp_prev, &spo2_ctx.red_ac_prev);

    int idx = spo2_ctx.samples_collected % BUFFER_SIZE;
    spo2_ctx.ir_buffer[idx] = ir_ac;
    spo2_ctx.red_buffer[idx] = red_ac;
    spo2_ctx.samples_collected++;

    if (spo2_ctx.samples_collected >= BUFFER_SIZE) {
        float ir_rms = 0.0f, red_rms = 0.0f;
        for (int i = 0; i < BUFFER_SIZE; i++) {
            ir_rms += spo2_ctx.ir_buffer[i] * spo2_ctx.ir_buffer[i];
            red_rms += spo2_ctx.red_buffer[i] * spo2_ctx.red_buffer[i];
        }
        ir_rms = sqrtf(ir_rms / BUFFER_SIZE);
        red_rms = sqrtf(red_rms / BUFFER_SIZE);

        float ir_dc_val = spo2_ctx.ir_dc;
        float red_dc_val = spo2_ctx.red_dc;
        if (ir_dc_val < 1.0f) ir_dc_val = 1.0f;
        if (red_dc_val < 1.0f) red_dc_val = 1.0f;

        if (ir_rms < SIGNAL_THRESHOLD || red_rms < SIGNAL_THRESHOLD) {
            spo2_ctx.data_ready = false;
        } else {
            float R = (red_rms / red_dc_val) / (ir_rms / ir_dc_val);
            float spo2 = 110.0f - 25.0f * R;
            if (spo2 < MIN_SPO2) spo2 = MIN_SPO2;
            if (spo2 > MAX_SPO2) spo2 = MAX_SPO2;
            spo2_ctx.spo2_value = (uint8_t)spo2;
            spo2_ctx.data_ready = true;
        }

        spo2_ctx.samples_collected = 0;
    }
    return ESP_OK;
}

esp_err_t spo2_get_result(spo2_result_t *result)
{
    if (!result) return ESP_ERR_INVALID_ARG;
    result->spo2 = spo2_ctx.spo2_value;
    result->data_ready = spo2_ctx.data_ready;
    return ESP_OK;
}
