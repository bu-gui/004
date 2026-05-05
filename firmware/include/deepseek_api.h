#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef struct {
    char date[16];
    uint32_t total_steps;
    float total_calories;
    float avg_heart_rate;
    float min_heart_rate;
    float max_heart_rate;
    uint8_t avg_spo2;
    float sleep_hours;
    uint8_t sleep_quality;
    uint32_t fall_count;
} daily_report_t;

typedef struct {
    char cmd_type[16];
    char message[256];
    char actuator[32];
    int count;
    int duration_ms;
} device_command_t;

esp_err_t deepseek_api_init(void);
esp_err_t deepseek_api_upload_data(const daily_report_t *report);
esp_err_t deepseek_api_get_command(device_command_t *cmd);
bool deepseek_api_is_ready(void);
