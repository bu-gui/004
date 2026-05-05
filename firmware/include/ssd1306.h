#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 64

esp_err_t ssd1306_init(void);
esp_err_t ssd1306_clear(void);
esp_err_t ssd1306_draw_pixel(uint8_t x, uint8_t y, bool on);
esp_err_t ssd1306_draw_char(uint8_t x, uint8_t y, char ch);
esp_err_t ssd1306_draw_string(uint8_t x, uint8_t y, const char *str);
esp_err_t ssd1306_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool fill);
esp_err_t ssd1306_display(void);
esp_err_t ssd1306_set_power(bool on);
