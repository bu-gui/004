#include <stdio.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
    max30102_write_reg(SPO2_CONFIG, 0x03);
    max30102_write_reg(LED1_PA, 0x24);
    max30102_write_reg(LED2_PA, 0x24);
    max30102_write_reg(MULTI_LED_CTRL1, 0x21);
    max30102_write_reg(MODE_CONFIG, 0x07);
    return ESP_OK;
}

esp_err_t max30102_read_fifo(uint32_t *ir, uint32_t *red)
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
        *ir = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
        *red = ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | data[5];
    }
    return ret;
}
