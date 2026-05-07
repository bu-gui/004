#include <stdio.h>
#include <math.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mpu6050.h"

#define MPU6050_ADDR            0x68

#define SMPLRT_DIV              0x19
#define CONFIG                  0x1A
#define GYRO_CONFIG             0x1B
#define ACCEL_CONFIG            0x1C
#define ACCEL_XOUT_H            0x3B
#define ACCEL_XOUT_L            0x3C
#define ACCEL_YOUT_H            0x3D
#define ACCEL_YOUT_L            0x3E
#define ACCEL_ZOUT_H            0x3F
#define ACCEL_ZOUT_L            0x40
#define TEMP_OUT_H              0x41
#define TEMP_OUT_L              0x42
#define GYRO_XOUT_H             0x43
#define GYRO_XOUT_L             0x44
#define GYRO_YOUT_H             0x45
#define GYRO_YOUT_L             0x46
#define GYRO_ZOUT_H             0x47
#define GYRO_ZOUT_L             0x48
#define PWR_MGMT_1              0x6B
#define PWR_MGMT_2              0x6C
#define WHO_AM_I                0x75

#define ACCEL_SCALE             16384.0f
#define GYRO_SCALE              131.0f

static float accel_offset_x = 0.0f;
static float accel_offset_y = 0.0f;
static float accel_offset_z = 0.0f;
static float gyro_offset_x = 0.0f;
static float gyro_offset_y = 0.0f;
static float gyro_offset_z = 0.0f;

static esp_err_t mpu6050_write_reg(uint8_t reg, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_ADDR << 1) | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(cmd, reg, 1);
    i2c_master_write_byte(cmd, data, 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t mpu6050_read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_ADDR << 1) | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(cmd, reg, 1);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MPU6050_ADDR << 1) | I2C_MASTER_READ, 1);
    for (size_t i = 0; i < len; i++) {
        i2c_master_read_byte(cmd, &data[i], (i == len - 1) ? 1 : 0);
    }
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t mpu6050_init(void)
{
    esp_err_t ret;
    uint8_t id = 0;
    ret = mpu6050_read_regs(WHO_AM_I, &id, 1);
    if (ret != ESP_OK) {
        printf("MPU6050 WHO_AM_I read failed\n");
        return ret;
    }
    printf("MPU6050 WHO_AM_I: 0x%02X\n", id);
    if (id != 0x68) {
        printf("MPU6050 not found!\n");
        return ESP_FAIL;
    }
    ret = mpu6050_write_reg(PWR_MGMT_1, 0x00);
    if (ret != ESP_OK) return ret;
    ret = mpu6050_write_reg(SMPLRT_DIV, 19);
    if (ret != ESP_OK) return ret;
    ret = mpu6050_write_reg(CONFIG, 0x03);
    if (ret != ESP_OK) return ret;
    ret = mpu6050_write_reg(GYRO_CONFIG, 0x00);
    if (ret != ESP_OK) return ret;
    ret = mpu6050_write_reg(ACCEL_CONFIG, 0x00);
    if (ret != ESP_OK) return ret;
    return ESP_OK;
}

esp_err_t mpu6050_read_all(mpu6050_data_t *data)
{
    uint8_t buf[14];
    esp_err_t ret = mpu6050_read_regs(ACCEL_XOUT_H, buf, 14);
    if (ret != ESP_OK) return ret;

    int16_t ax = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t ay = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t az = (int16_t)((buf[4] << 8) | buf[5]);
    int16_t t  = (int16_t)((buf[6] << 8) | buf[7]);
    int16_t gx = (int16_t)((buf[8] << 8) | buf[9]);
    int16_t gy = (int16_t)((buf[10] << 8) | buf[11]);
    int16_t gz = (int16_t)((buf[12] << 8) | buf[13]);

    data->accel_x = (float)ax / ACCEL_SCALE - accel_offset_x;
    data->accel_y = (float)ay / ACCEL_SCALE - accel_offset_y;
    data->accel_z = (float)az / ACCEL_SCALE - accel_offset_z;
    data->temperature = (float)t / 340.0f + 36.53f;
    data->gyro_x = (float)gx / GYRO_SCALE - gyro_offset_x;
    data->gyro_y = (float)gy / GYRO_SCALE - gyro_offset_y;
    data->gyro_z = (float)gz / GYRO_SCALE - gyro_offset_z;
    return ESP_OK;
}

esp_err_t mpu6050_calibrate(void)
{
    mpu6050_data_t data;
    float sum_ax = 0.0f, sum_ay = 0.0f, sum_az = 0.0f;
    float sum_gx = 0.0f, sum_gy = 0.0f, sum_gz = 0.0f;

    for (int i = 0; i < 100; i++) {
        esp_err_t ret = mpu6050_read_all(&data);
        if (ret != ESP_OK) return ret;
        sum_ax += data.accel_x;
        sum_ay += data.accel_y;
        sum_az += data.accel_z;
        sum_gx += data.gyro_x;
        sum_gy += data.gyro_y;
        sum_gz += data.gyro_z;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    accel_offset_x = sum_ax / 100.0f;
    accel_offset_y = sum_ay / 100.0f;
    accel_offset_z = sum_az / 100.0f;
    gyro_offset_x = sum_gx / 100.0f;
    gyro_offset_y = sum_gy / 100.0f;
    gyro_offset_z = sum_gz / 100.0f;

    printf("Accel offsets: x=%.4f y=%.4f z=%.4f\n", accel_offset_x, accel_offset_y, accel_offset_z);
    printf("Gyro offsets: x=%.4f y=%.4f z=%.4f\n", gyro_offset_x, gyro_offset_y, gyro_offset_z);
    return ESP_OK;
}

esp_err_t mpu6050_get_offsets(int16_t *out_accel_x, int16_t *out_accel_y, int16_t *out_accel_z,
                              int16_t *out_gyro_x, int16_t *out_gyro_y, int16_t *out_gyro_z)
{
    *out_accel_x = (int16_t)(accel_offset_x * ACCEL_SCALE);
    *out_accel_y = (int16_t)(accel_offset_y * ACCEL_SCALE);
    *out_accel_z = (int16_t)(accel_offset_z * ACCEL_SCALE);
    *out_gyro_x = (int16_t)(gyro_offset_x * GYRO_SCALE);
    *out_gyro_y = (int16_t)(gyro_offset_y * GYRO_SCALE);
    *out_gyro_z = (int16_t)(gyro_offset_z * GYRO_SCALE);
    return ESP_OK;
}

esp_err_t mpu6050_set_low_power(bool enable)
{
    if (enable) {
        return mpu6050_write_reg(PWR_MGMT_2, 0x07);
    } else {
        return mpu6050_write_reg(PWR_MGMT_2, 0x00);
    }
}
