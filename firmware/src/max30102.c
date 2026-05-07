#include <stdio.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "max30102.h"

#define MAX30102_ADDR           0x57

#define INTR_STATUS_1           0x00
#define INTR_STATUS_2           0x01
#define INTR_ENABLE_1           0x02
#define INTR_ENABLE_2           0x03
#define FIFO_WR_PTR             0x04
#define OVF_COUNTER             0x05
#define FIFO_RD_PTR             0x06
#define FIFO_DATA               0x07
#define FIFO_CONFIG             0x08
#define MODE_CONFIG             0x09
#define SPO2_CONFIG             0x0A
#define LED1_PA                 0x0C
#define LED2_PA                 0x0D
#define MULTI_LED_CTRL1         0x11
#define MULTI_LED_CTRL2         0x12
#define PART_ID                 0xFF

static esp_err_t max30102_write_reg(uint8_t reg, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX30102_ADDR << 1) | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(cmd, reg, 1);
    i2c_master_write_byte(cmd, data, 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t max30102_read_reg(uint8_t reg, uint8_t *data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX30102_ADDR << 1) | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(cmd, reg, 1);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX30102_ADDR << 1) | I2C_MASTER_READ, 1);
    i2c_master_read_byte(cmd, data, 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t max30102_init(void)
{
    uint8_t id = 0;
    max30102_read_reg(PART_ID, &id);
    printf("MAX30102 PART_ID: 0x%02X\n", id);
    if (id != 0x15) {
        printf("MAX30102 not found!\n");
        return ESP_FAIL;
    }
    max30102_write_reg(MODE_CONFIG, 0x40);
    vTaskDelay(pdMS_TO_TICKS(100));
    max30102_write_reg(FIFO_CONFIG, 0x0F);
    // MAX1: 采样率改为50Hz (SPO2_CONFIG bit2=1)
    max30102_write_reg(SPO2_CONFIG, 0x07);
    // MAX3: LED电流写后读回验证
    uint8_t verify = 0;
    max30102_write_reg(LED1_PA, 0x24);
    max30102_read_reg(LED1_PA, &verify);
    if (verify != 0x24) {
        printf("MAX30102 LED1_PA write verify failed: got 0x%02X\n", verify);
    }
    max30102_write_reg(LED2_PA, 0x24);
    max30102_write_reg(MULTI_LED_CTRL1, 0x21);
    max30102_write_reg(MODE_CONFIG, 0x07);
    // MAX4: 等待传感器稳定
    vTaskDelay(pdMS_TO_TICKS(150));
    return ESP_OK;
}

esp_err_t max30102_read_fifo(max30102_sample_t *sample)
{
    uint8_t data[6] = {0};
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX30102_ADDR << 1) | I2C_MASTER_WRITE, 1);
    i2c_master_write_byte(cmd, FIFO_DATA, 1);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MAX30102_ADDR << 1) | I2C_MASTER_READ, 1);
    i2c_master_read(cmd, data, 5, 0);
    i2c_master_read_byte(cmd, &data[5], 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_OK) {
        sample->ir_raw = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
        sample->red_raw = ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | data[5];
        sample->data_valid = true;
        // MAX2: 检查溢出标志 (寄存器0x08 bit6)
        uint8_t fifo_config = 0;
        max30102_read_reg(FIFO_CONFIG, &fifo_config);
        if (fifo_config & 0x40) {
            printf("MAX30102 FIFO overflow detected, clearing FIFO\n");
            max30102_write_reg(FIFO_WR_PTR, 0x00);
            max30102_write_reg(OVF_COUNTER, 0x00);
            max30102_write_reg(FIFO_RD_PTR, 0x00);
        }
    } else {
        sample->data_valid = false;
    }
    return ret;
}

esp_err_t max30102_set_led_current(uint8_t ir_ma, uint8_t red_ma)
{
    esp_err_t ret = max30102_write_reg(LED1_PA, ir_ma);
    if (ret != ESP_OK) return ret;
    return max30102_write_reg(LED2_PA, red_ma);
}

esp_err_t max30102_set_sample_rate(uint8_t rate)
{
    uint8_t config = 0;
    esp_err_t ret = max30102_read_reg(SPO2_CONFIG, &config);
    if (ret != ESP_OK) return ret;
    config = (config & 0xE3) | ((rate & 0x07) << 2);
    return max30102_write_reg(SPO2_CONFIG, config);
}

esp_err_t max30102_reset(void)
{
    return max30102_write_reg(MODE_CONFIG, 0x40);
}
