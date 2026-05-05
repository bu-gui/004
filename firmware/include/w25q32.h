#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

#define W25Q32_PAGE_SIZE     256
#define W25Q32_SECTOR_SIZE   4096
#define W25Q32_TOTAL_SIZE    (4 * 1024 * 1024)

esp_err_t w25q32_init(void);
esp_err_t w25q32_read(uint32_t addr, uint8_t *data, size_t len);
esp_err_t w25q32_write(uint32_t addr, const uint8_t *data, size_t len);
esp_err_t w25q32_erase_sector(uint32_t sector_addr);
esp_err_t w25q32_erase_all(void);
uint32_t w25q32_get_size(void);
