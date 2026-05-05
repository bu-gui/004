#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

#define I2C_BUS_SDA_GPIO     18
#define I2C_BUS_SCL_GPIO     19

#define I2C_ADDR_MAX30102    0x57
#define I2C_ADDR_MPU6050     0x68
#define I2C_ADDR_SSD1306     0x3C

esp_err_t i2c_bus_init(void);
esp_err_t i2c_bus_write(uint8_t dev_addr, const uint8_t *data, size_t len);
esp_err_t i2c_bus_read(uint8_t dev_addr, uint8_t *data, size_t len);
esp_err_t i2c_bus_write_byte(uint8_t dev_addr, uint8_t reg, uint8_t value);
esp_err_t i2c_bus_read_byte(uint8_t dev_addr, uint8_t reg, uint8_t *value);
esp_err_t i2c_bus_scan(void);
