#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "w25q32.h"

#define PIN_CS      10
#define PIN_MOSI    11
#define PIN_MISO    12
#define PIN_SCK     13

#define WRITE_ENABLE        0x06
#define WRITE_DISABLE       0x04
#define READ_STATUS_REG_1   0x05
#define READ_STATUS_REG_2   0x35
#define PAGE_PROGRAM        0x02
#define READ_DATA           0x03
#define SECTOR_ERASE        0x20
#define CHIP_ERASE          0xC7
#define READ_DEVICE_ID      0x9F

#define WEL_BIT             (1 << 1)
#define BUSY_BIT            (1 << 0)

#define WAIT_BUSY_TIMEOUT_MS    500
#define WRITE_ENABLE_RETRY_MAX  3
#define MUTEX_TIMEOUT_MS        1000

static const char *TAG = "w25q32";
static spi_device_handle_t spi_dev;
static SemaphoreHandle_t w25q32_mutex = NULL;

static void w25q32_cs_low(void)
{
    gpio_set_level(PIN_CS, 0);
}

static void w25q32_cs_high(void)
{
    gpio_set_level(PIN_CS, 1);
}

static uint8_t w25q32_rw_byte(uint8_t data)
{
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
        .rx_buffer = &data,
    };
    spi_device_transmit(spi_dev, &t);
    return data;
}

static void w25q32_read_status_reg1(uint8_t *status)
{
    w25q32_cs_low();
    w25q32_rw_byte(READ_STATUS_REG_1);
    *status = w25q32_rw_byte(0xFF);
    w25q32_cs_high();
}

static esp_err_t w25q32_write_enable(void)
{
    esp_err_t ret = ESP_FAIL;
    for (int retry = 0; retry < WRITE_ENABLE_RETRY_MAX; retry++) {
        w25q32_cs_low();
        w25q32_rw_byte(WRITE_ENABLE);
        w25q32_cs_high();

        uint8_t status;
        w25q32_read_status_reg1(&status);
        if (status & WEL_BIT) {
            ret = ESP_OK;
            break;
        }
        ESP_LOGW(TAG, "WEL not set after write enable (retry %d, status=0x%02X)", retry + 1, status);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return ret;
}

static esp_err_t w25q32_wait_busy(void)
{
    uint8_t status;
    int elapsed_ms = 0;
    do {
        w25q32_read_status_reg1(&status);
        if (!(status & BUSY_BIT)) {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
        elapsed_ms += 1;
    } while (elapsed_ms < WAIT_BUSY_TIMEOUT_MS);

    ESP_LOGE(TAG, "Wait busy timeout after %dms, status=0x%02X", WAIT_BUSY_TIMEOUT_MS, status);
    return ESP_ERR_TIMEOUT;
}

esp_err_t w25q32_init(void)
{
    esp_err_t ret;

    if (w25q32_mutex == NULL) {
        w25q32_mutex = xSemaphoreCreateMutex();
        if (w25q32_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create mutex");
            return ESP_ERR_NO_MEM;
        }
    }

    gpio_config_t cs_io = {
        .pin_bit_mask = (1ULL << PIN_CS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&cs_io);
    w25q32_cs_high();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 20 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1,
    };

    ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(ret));
        return ret;
    }

    w25q32_cs_low();
    w25q32_rw_byte(READ_DEVICE_ID);
    uint8_t manufacturer = w25q32_rw_byte(0xFF);
    uint8_t mem_type = w25q32_rw_byte(0xFF);
    uint8_t capacity = w25q32_rw_byte(0xFF);
    w25q32_cs_high();

    if (manufacturer != 0xEF || mem_type != 0x40 || capacity != 0x16) {
        ESP_LOGW(TAG, "Unexpected W25Q32 ID: 0x%02X 0x%02X 0x%02X",
                 manufacturer, mem_type, capacity);
    } else {
        ESP_LOGI(TAG, "W25Q32 detected, ID: 0x%02X 0x%02X 0x%02X",
                 manufacturer, mem_type, capacity);
    }

    return ESP_OK;
}

esp_err_t w25q32_read(uint32_t addr, uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (addr + len > W25Q32_TOTAL_SIZE) {
        return ESP_ERR_INVALID_SIZE;
    }

    if (w25q32_mutex == NULL || xSemaphoreTake(w25q32_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    w25q32_cs_low();
    w25q32_rw_byte(READ_DATA);
    w25q32_rw_byte((addr >> 16) & 0xFF);
    w25q32_rw_byte((addr >> 8) & 0xFF);
    w25q32_rw_byte(addr & 0xFF);
    for (size_t i = 0; i < len; i++) {
        data[i] = w25q32_rw_byte(0xFF);
    }
    w25q32_cs_high();

    xSemaphoreGive(w25q32_mutex);
    return ESP_OK;
}

esp_err_t w25q32_write(uint32_t addr, const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (addr + len > W25Q32_TOTAL_SIZE) {
        return ESP_ERR_INVALID_SIZE;
    }

    if (w25q32_mutex == NULL || xSemaphoreTake(w25q32_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    size_t remaining = len;
    uint32_t current_addr = addr;
    const uint8_t *p_data = data;

    while (remaining > 0) {
        /* Calculate bytes available within current page (256-byte boundary) */
        uint32_t page_offset = current_addr % W25Q32_PAGE_SIZE;
        size_t chunk_size = W25Q32_PAGE_SIZE - page_offset;
        if (chunk_size > remaining) {
            chunk_size = remaining;
        }

        esp_err_t ret = w25q32_write_enable();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Write enable failed at addr 0x%08X", (unsigned int)current_addr);
            xSemaphoreGive(w25q32_mutex);
            return ret;
        }

        w25q32_cs_low();
        w25q32_rw_byte(PAGE_PROGRAM);
        w25q32_rw_byte((current_addr >> 16) & 0xFF);
        w25q32_rw_byte((current_addr >> 8) & 0xFF);
        w25q32_rw_byte(current_addr & 0xFF);
        for (size_t i = 0; i < chunk_size; i++) {
            w25q32_rw_byte(p_data[i]);
        }
        w25q32_cs_high();

        ret = w25q32_wait_busy();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Wait busy timeout after page program at addr 0x%08X", (unsigned int)current_addr);
            xSemaphoreGive(w25q32_mutex);
            return ESP_ERR_TIMEOUT;
        }

        current_addr += chunk_size;
        p_data += chunk_size;
        remaining -= chunk_size;
    }

    xSemaphoreGive(w25q32_mutex);
    return ESP_OK;
}

esp_err_t w25q32_erase_sector(uint32_t addr)
{
    if (w25q32_mutex == NULL || xSemaphoreTake(w25q32_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t ret = w25q32_write_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write enable failed for sector erase");
        xSemaphoreGive(w25q32_mutex);
        return ret;
    }

    w25q32_cs_low();
    w25q32_rw_byte(SECTOR_ERASE);
    w25q32_rw_byte((addr >> 16) & 0xFF);
    w25q32_rw_byte((addr >> 8) & 0xFF);
    w25q32_rw_byte(addr & 0xFF);
    w25q32_cs_high();

    ret = w25q32_wait_busy();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wait busy timeout after sector erase at addr 0x%08X", (unsigned int)addr);
    }

    xSemaphoreGive(w25q32_mutex);
    return ret;
}

esp_err_t w25q32_erase_all(void)
{
    if (w25q32_mutex == NULL || xSemaphoreTake(w25q32_mutex, pdMS_TO_TICKS(MUTEX_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t ret = w25q32_write_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write enable failed for chip erase");
        xSemaphoreGive(w25q32_mutex);
        return ret;
    }

    w25q32_cs_low();
    w25q32_rw_byte(CHIP_ERASE);
    w25q32_cs_high();

    ret = w25q32_wait_busy();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Wait busy timeout after chip erase");
    }

    xSemaphoreGive(w25q32_mutex);
    return ret;
}

uint32_t w25q32_get_size(void)
{
    return W25Q32_TOTAL_SIZE;
}
