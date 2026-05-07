#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef struct __attribute__((packed)) {
    float heart_rate;
    uint8_t spo2;
    uint32_t steps;
    float calories;
    uint8_t motion_type;
    uint8_t fall_detected;
    float battery_voltage;
} ble_data_packet_t;

esp_err_t ble_init(void);
esp_err_t ble_send_data(const ble_data_packet_t *packet);
bool ble_is_connected(void);
esp_err_t ble_start_advertising(void);
