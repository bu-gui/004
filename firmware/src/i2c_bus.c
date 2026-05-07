#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "i2c_bus.h"

#define I2C_MASTER_SCL_IO           GPIO_NUM_19
#define I2C_MASTER_SDA_IO           GPIO_NUM_18
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define ACK_CHECK_EN                0x01
#define ACK_CHECK_DIS               0x00
#define ACK_VAL                     0x00
#define NACK_VAL                    0x01

#define RETRY_MAX_COUNT             3
#define RETRY_INTERVAL_MS           10
#define MUTEX_TIMEOUT_MS            1000

static const char *TAG = "i2c_bus";
static SemaphoreHandle_t i2c_mutex = NULL;

esp_err_t i2c_bus_init(void)
{
    esp_err_t ret;

    if (i2c_mutex == NULL) {
        i2c_mutex = xSemaphoreCreateMutex();
        if (i2c_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create I2C mutex");
            return ESP_ERR_NO_MEM;
        }
    }

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                             I2C_MASTER_TX_BUF_DISABLE, I2C_MASTER_RX_BUF_DISABLE, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2C bus initialized successfully");
    return ESP_OK;
}

static esp_err_t i2c_bus_write_internal(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *data, size_t len)
{
    esp_err_t ret;
    for (int retry = 0; retry < RETRY_MAX_COUNT; retry++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
        if (len > 0) {
            i2c_master_write(cmd, data, len, ACK_CHECK_EN);
        }
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            return ESP_OK;
        }
        if (retry < RETRY_MAX_COUNT - 1) {
            vTaskDelay(pdMS_TO_TICKS(RETRY_INTERVAL_MS));
        }
    }
    return ret;
}

static esp_err_t i2c_bus_read_internal(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len)
{
    esp_err_t ret;
    for (int retry = 0; retry < RETRY_MAX_COUNT; retry++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
        if (len > 1) {
            i2c_master_read(cmd, data, len - 1, ACK_VAL);
        }
        i2c_master_read_byte(cmd, &data[len - 1], NACK_VAL);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            return ESP_OK;
        }
        if (retry < RETRY_MAX_COUNT - 1) {
            vTaskDelay(pdMS_TO_TICKS(RETRY_INTERVAL_MS));
        }
    }
    return ret;
}

esp_err_t i2c_bus_write_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    esp_err_t ret;
    if (i2c_mutex == NULL || xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    ret = i2c_bus_write_internal(dev_addr, reg_addr, &data, 1);
    xSemaphoreGive(i2c_mutex);
    return ret;
}

esp_err_t i2c_bus_read_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data)
{
    esp_err_t ret;
    if (i2c_mutex == NULL || xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    ret = i2c_bus_read_internal(dev_addr, reg_addr, data, 1);
    xSemaphoreGive(i2c_mutex);
    return ret;
}

esp_err_t i2c_bus_write(uint8_t dev_addr, const uint8_t *data, size_t len)
{
    esp_err_t ret;
    if (i2c_mutex == NULL || xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    for (int retry = 0; retry < RETRY_MAX_COUNT; retry++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        if (len > 0) {
            i2c_master_write(cmd, data, len, ACK_CHECK_EN);
        }
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            xSemaphoreGive(i2c_mutex);
            return ESP_OK;
        }
        if (retry < RETRY_MAX_COUNT - 1) {
            vTaskDelay(pdMS_TO_TICKS(RETRY_INTERVAL_MS));
        }
    }

    xSemaphoreGive(i2c_mutex);
    return ret;
}

esp_err_t i2c_bus_read(uint8_t dev_addr, uint8_t *data, size_t len)
{
    esp_err_t ret;
    if (i2c_mutex == NULL || xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    for (int retry = 0; retry < RETRY_MAX_COUNT; retry++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
        if (len > 1) {
            i2c_master_read(cmd, data, len - 1, ACK_VAL);
        }
        i2c_master_read_byte(cmd, &data[len - 1], NACK_VAL);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            xSemaphoreGive(i2c_mutex);
            return ESP_OK;
        }
        if (retry < RETRY_MAX_COUNT - 1) {
            vTaskDelay(pdMS_TO_TICKS(RETRY_INTERVAL_MS));
        }
    }

    xSemaphoreGive(i2c_mutex);
    return ret;
}

esp_err_t i2c_bus_scan(void)
{
#if CONFIG_I2C_SCAN_ENABLE
    if (i2c_mutex == NULL || xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGI(TAG, "I2C bus scanning...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Found device at 0x%02X", addr);
        }
    }
    ESP_LOGI(TAG, "I2C bus scan done.");

    xSemaphoreGive(i2c_mutex);
    return ESP_OK;
#else
    ESP_LOGW(TAG, "I2C scan disabled (enable CONFIG_I2C_SCAN_ENABLE)");
    return ESP_ERR_NOT_SUPPORTED;
#endif
}
