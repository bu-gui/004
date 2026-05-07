#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float temperature;
} mpu6050_data_t;

esp_err_t mpu6050_init(void);
esp_err_t mpu6050_read_all(mpu6050_data_t *data);
esp_err_t mpu6050_calibrate(void);
esp_err_t mpu6050_get_offsets(int16_t *accel_offset_x, int16_t *accel_offset_y, int16_t *accel_offset_z,
                              int16_t *gyro_offset_x, int16_t *gyro_offset_y, int16_t *gyro_offset_z);
esp_err_t mpu6050_set_low_power(bool enable);
