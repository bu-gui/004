#include <stdio.h>
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

static spi_device_handle_t spi_dev;

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

static void w25q32_write_enable(void)
{
    w25q32_cs_low();
    w25q32_rw_byte(WRITE_ENABLE);
    w25q32_cs_high();
}

static void w25q32_wait_busy(void)
{
    uint8_t status;
    do {
        w25q32_cs_low();
        w25q32_rw_byte(READ_STATUS_REG_1);
        status = w25q32_rw_byte(0xFF);
        w25q32_cs_high();
        vTaskDelay(pdMS_TO_TICKS(1));
    } while (status & 0x01);
}

esp_err_t w25q32_init(void)
{
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

    spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_dev);

    w25q32_cs_low();
    w25q32_rw_byte(READ_DEVICE_ID);
    uint8_t manufacturer = w25q32_rw_byte(0xFF);
    uint8_t mem_type = w25q32_rw_byte(0xFF);
    uint8_t capacity = w25q32_rw_byte(0xFF);
    w25q32_cs_high();

    printf("W25Q32 ID: 0x%02X 0x%02X 0x%02X\n", manufacturer, mem_type, capacity);
    return ESP_OK;
}

esp_err_t w25q32_read(uint32_t addr, uint8_t *data, size_t len)
{
    w25q32_cs_low();
    w25q32_rw_byte(READ_DATA);
    w25q32_rw_byte((addr >> 16) & 0xFF);
    w25q32_rw_byte((addr >> 8) & 0xFF);
    w25q32_rw_byte(addr & 0xFF);
    for (size_t i = 0; i < len; i++) {
        data[i] = w25q32_rw_byte(0xFF);
    }
    w25q32_cs_high();
    return ESP_OK;
}

esp_err_t w25q32_write(uint32_t addr, const uint8_t *data, size_t len)
{
    w25q32_write_enable();
    w25q32_cs_low();
    w25q32_rw_byte(PAGE_PROGRAM);
    w25q32_rw_byte((addr >> 16) & 0xFF);
    w25q32_rw_byte((addr >> 8) & 0xFF);
    w25q32_rw_byte(addr & 0xFF);
    for (size_t i = 0; i < len; i++) {
        w25q32_rw_byte(data[i]);
    }
    w25q32_cs_high();
    w25q32_wait_busy();
    return ESP_OK;
}

esp_err_t w25q32_erase_sector(uint32_t addr)
{
    w25q32_write_enable();
    w25q32_cs_low();
    w25q32_rw_byte(SECTOR_ERASE);
    w25q32_rw_byte((addr >> 16) & 0xFF);
    w25q32_rw_byte((addr >> 8) & 0xFF);
    w25q32_rw_byte(addr & 0xFF);
    w25q32_cs_high();
    w25q32_wait_busy();
    return ESP_OK;
}

esp_err_t w25q32_erase_all(void)
{
    w25q32_write_enable();
    w25q32_cs_low();
    w25q32_rw_byte(CHIP_ERASE);
    w25q32_cs_high();
    w25q32_wait_busy();
    return ESP_OK;
}
