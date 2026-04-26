#ifndef SH1106_H
#define SH1106_H

#include "driver/i2c.h"
#include "esp_err.h"

// I2C 配置
#define SH1106_I2C_NUM      I2C_NUM_0
#define SH1106_I2C_SDA_PIN  8
#define SH1106_I2C_SCL_PIN  9
#define SH1106_I2C_FREQ_HZ  400000
#define SH1106_ADDR         0x3C    // 或 0x3D

// 显示参数
#define SH1106_WIDTH        128
#define SH1106_HEIGHT       64
#define SH1106_PAGES        8
#define SH1106_OFFSET       2       // SH1106 关键: 列偏移 2

// 颜色定义
#define SH1106_COLOR_BLACK  0
#define SH1106_COLOR_WHITE  1
#define SH1106_COLOR_INVERT 2

// 初始化
esp_err_t sh1106_init(void);
esp_err_t sh1106_deinit(void);

// 显示控制
esp_err_t sh1106_display_on(void);
esp_err_t sh1106_display_off(void);
esp_err_t sh1106_set_contrast(uint8_t contrast);
esp_err_t sh1106_set_brightness(uint8_t brightness); // 0-255
esp_err_t sh1106_invert_display(bool invert);
esp_err_t sh1106_flip_display(bool horizontal, bool vertical);

// 显存操作
esp_err_t sh1106_clear(void);
esp_err_t sh1106_fill(uint8_t pattern);
esp_err_t sh1106_refresh(void);      // 局部刷新，仅更新修改区域
esp_err_t sh1106_refresh_full(void); // 强制全屏刷新

// 像素绘制
esp_err_t sh1106_draw_pixel(int16_t x, int16_t y, uint8_t color);
esp_err_t sh1106_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
esp_err_t sh1106_draw_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t color);
esp_err_t sh1106_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t color);
esp_err_t sh1106_draw_circle(int16_t x, int16_t y, uint16_t r, uint8_t color);
esp_err_t sh1106_draw_bitmap(int16_t x, int16_t y, const uint8_t *bitmap, uint16_t w, uint16_t h);

// 文字显示
esp_err_t sh1106_draw_char(int16_t x, int16_t y, char c, uint8_t color);
esp_err_t sh1106_draw_string(int16_t x, int16_t y, const char *str, uint8_t color);
void sh1106_set_font(const uint8_t *font_data, uint8_t font_width, uint8_t font_height);

// 工具函数
void sh1106_get_text_bounds(const char *str, int16_t x, int16_t y, 
                            int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);

#endif