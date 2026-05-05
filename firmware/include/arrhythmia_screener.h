#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef enum {
    ARRHYTHMIA_NORMAL          = 0,
    ARRHYTHMIA_SUSPECT_AF     = 1,
    ARRHYTHMIA_SUSPECT_PVC    = 2,
    ARRHYTHMIA_INSUFFICIENT_DATA = 3
} arrhythmia_result_t;

esp_err_t arrhythmia_screener_init(void);
esp_err_t arrhythmia_screener_feed_rr_interval(uint32_t rr_ms);
arrhythmia_result_t arrhythmia_screener_analyze(void);
uint32_t arrhythmia_screener_get_sdnn(void);
esp_err_t arrhythmia_screener_reset(void);
