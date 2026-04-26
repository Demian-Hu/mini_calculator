#include "sh1106.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>

static const char *TAG = "SH1106";

// 显存缓冲区: 8页 × 128列
static uint8_t sh1106_buffer[SH1106_PAGES][SH1106_WIDTH];
static bool buffer_dirty[SH1106_PAGES] = {false};
static bool full_refresh = true;

// 当前字体 (默认 8x6)
static const uint8_t *current_font = NULL;
static uint8_t font_w = 6, font_h = 8;

// 默认 8x6 ASCII 字体 (部分字符)
static const uint8_t font_8x6[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, // 32: Space
    {0x00,0x00,0x5F,0x00,0x00,0x00}, // 33: !
    {0x00,0x07,0x00,0x07,0x00,0x00}, // 34: "
    {0x14,0x7F,0x14,0x7F,0x14,0x00}, // 35: #
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, // 36: $
    {0x23,0x13,0x08,0x64,0x62,0x00}, // 37: %
    {0x36,0x49,0x55,0x22,0x50,0x00}, // 38: &
    {0x00,0x05,0x03,0x00,0x00,0x00}, // 39: '
    {0x00,0x1C,0x22,0x41,0x00,0x00}, // 40: (
    {0x00,0x41,0x22,0x1C,0x00,0x00}, // 41: )
    {0x08,0x2A,0x1C,0x2A,0x08,0x00}, // 42: *
    {0x08,0x08,0x3E,0x08,0x08,0x00}, // 43: +
    {0x00,0x50,0x30,0x00,0x00,0x00}, // 44: ,
    {0x08,0x08,0x08,0x08,0x08,0x00}, // 45: -
    {0x00,0x60,0x60,0x00,0x00,0x00}, // 46: .
    {0x20,0x10,0x08,0x04,0x02,0x00}, // 47: /
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, // 48: 0
    {0x00,0x42,0x7F,0x40,0x00,0x00}, // 49: 1
    {0x42,0x61,0x51,0x49,0x46,0x00}, // 50: 2
    {0x21,0x41,0x45,0x4B,0x31,0x00}, // 51: 3
    {0x18,0x14,0x12,0x7F,0x10,0x00}, // 52: 4
    {0x27,0x45,0x45,0x45,0x39,0x00}, // 53: 5
    {0x3C,0x4A,0x49,0x49,0x30,0x00}, // 54: 6
    {0x01,0x71,0x09,0x05,0x03,0x00}, // 55: 7
    {0x36,0x49,0x49,0x49,0x36,0x00}, // 56: 8
    {0x06,0x49,0x49,0x29,0x1E,0x00}, // 57: 9
    {0x00,0x36,0x36,0x00,0x00,0x00}, // 58: :
    {0x00,0x56,0x36,0x00,0x00,0x00}, // 59: ;
    {0x00,0x08,0x14,0x22,0x41,0x00}, // 60: <
    {0x14,0x14,0x14,0x14,0x14,0x00}, // 61: =
    {0x41,0x22,0x14,0x08,0x00,0x00}, // 62: >
    {0x02,0x01,0x51,0x09,0x06,0x00}, // 63: ?
    {0x32,0x49,0x79,0x41,0x3E,0x00}, // 64: @
    {0x7E,0x11,0x11,0x11,0x7E,0x00}, // 65: A
    {0x7F,0x49,0x49,0x49,0x36,0x00}, // 66: B
    {0x3E,0x41,0x41,0x41,0x22,0x00}, // 67: C
    {0x7F,0x41,0x41,0x22,0x1C,0x00}, // 68: D
    {0x7F,0x49,0x49,0x49,0x41,0x00}, // 69: E
    {0x7F,0x09,0x09,0x01,0x01,0x00}, // 70: F
    {0x3E,0x41,0x49,0x49,0x7A,0x00}, // 71: G
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, // 72: H
    {0x00,0x41,0x7F,0x41,0x00,0x00}, // 73: I
    {0x20,0x40,0x41,0x3F,0x01,0x00}, // 74: J
    {0x00,0x7F,0x10,0x28,0x44,0x00}, // 75: K
    {0x7F,0x40,0x40,0x40,0x40,0x00}, // 76: L
    {0x7F,0x02,0x04,0x02,0x7F,0x00}, // 77: M
    {0x7F,0x04,0x08,0x10,0x7F,0x00}, // 78: N
    {0x3E,0x41,0x41,0x41,0x3E,0x00}, // 79: O
    {0x7F,0x09,0x09,0x09,0x06,0x00}, // 80: P
    {0x3E,0x41,0x51,0x21,0x5E,0x00}, // 81: Q
    {0x7F,0x09,0x19,0x29,0x46,0x00}, // 82: R
    {0x46,0x49,0x49,0x49,0x31,0x00}, // 83: S
    {0x01,0x01,0x7F,0x01,0x01,0x00}, // 84: T
    {0x3F,0x40,0x40,0x40,0x3F,0x00}, // 85: U
    {0x1F,0x20,0x40,0x20,0x1F,0x00}, // 86: V
    {0x7F,0x20,0x18,0x20,0x7F,0x00}, // 87: W
    {0x63,0x14,0x08,0x14,0x63,0x00}, // 88: X
    {0x03,0x04,0x78,0x04,0x03,0x00}, // 89: Y
    {0x61,0x51,0x49,0x45,0x43,0x00}, // 90: Z
    {0x00,0x00,0x7F,0x41,0x41,0x00}, // 91: [
    {0x02,0x04,0x08,0x10,0x20,0x00}, // 92: backslash (修复: 不写 \ 字符)
    {0x41,0x41,0x7F,0x00,0x00,0x00}, // 93: ]
    {0x04,0x02,0x01,0x02,0x04,0x00}, // 94: ^
    {0x40,0x40,0x40,0x40,0x40,0x00}, // 95: _
    {0x00,0x01,0x02,0x04,0x00,0x00}, // 96: `
    {0x20,0x54,0x54,0x54,0x78,0x00}, // 97: a
    {0x7F,0x48,0x44,0x44,0x38,0x00}, // 98: b
    {0x38,0x44,0x44,0x44,0x20,0x00}, // 99: c
    {0x38,0x44,0x44,0x48,0x7F,0x00}, // 100: d
    {0x38,0x54,0x54,0x54,0x18,0x00}, // 101: e
    {0x08,0x7E,0x09,0x01,0x02,0x00}, // 102: f
    {0x08,0x14,0x54,0x54,0x3C,0x00}, // 103: g
    {0x7F,0x08,0x04,0x04,0x78,0x00}, // 104: h
    {0x00,0x44,0x7D,0x40,0x00,0x00}, // 105: i
    {0x20,0x40,0x44,0x3D,0x00,0x00}, // 106: j
    {0x00,0x7F,0x10,0x28,0x44,0x00}, // 107: k
    {0x00,0x41,0x7F,0x40,0x00,0x00}, // 108: l
    {0x7C,0x04,0x18,0x04,0x78,0x00}, // 109: m
    {0x7C,0x08,0x04,0x04,0x78,0x00}, // 110: n
    {0x38,0x44,0x44,0x44,0x38,0x00}, // 111: o
    {0x7C,0x14,0x14,0x14,0x08,0x00}, // 112: p
    {0x08,0x14,0x14,0x18,0x7C,0x00}, // 113: q
    {0x7C,0x08,0x04,0x04,0x08,0x00}, // 114: r
    {0x48,0x54,0x54,0x54,0x20,0x00}, // 115: s
    {0x04,0x3F,0x44,0x40,0x20,0x00}, // 116: t
    {0x3C,0x40,0x40,0x20,0x7C,0x00}, // 117: u
    {0x1C,0x20,0x40,0x20,0x1C,0x00}, // 118: v
    {0x3C,0x40,0x30,0x40,0x3C,0x00}, // 119: w
    {0x44,0x28,0x10,0x28,0x44,0x00}, // 120: x
    {0x0C,0x50,0x50,0x50,0x3C,0x00}, // 121: y
    {0x44,0x64,0x54,0x4C,0x44,0x00}, // 122: z
    {0x00,0x08,0x36,0x41,0x00,0x00}, // 123: {
    {0x00,0x00,0x7F,0x00,0x00,0x00}, // 124: |
    {0x00,0x41,0x36,0x08,0x00,0x00}, // 125: }
    {0x08,0x08,0x2A,0x1C,0x08,0x00}, // 126: ~
};

// I2C 底层操作
static esp_err_t sh1106_write_cmd(uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd};  // Control byte: Co=0, D/C#=0
    return i2c_master_write_to_device(SH1106_I2C_NUM, SH1106_ADDR, 
                                      data, 2, pdMS_TO_TICKS(100));
}

// 删除未使用的函数 sh1106_write_data_byte，或者如果需要保留，添加 __attribute__((unused))
/*
static esp_err_t sh1106_write_data_byte(uint8_t data_byte) {
    uint8_t data[2] = {0x40, data_byte};  // Control byte: Co=0, D/C#=1
    return i2c_master_write_to_device(SH1106_I2C_NUM, SH1106_ADDR, 
                                      data, 2, pdMS_TO_TICKS(100));
}
*/

static esp_err_t sh1106_write_data_multi(const uint8_t *data, size_t len) {
    if (len == 0) return ESP_OK;
    
    // 使用 I2C 命令链接更高效
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SH1106_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x40, true);  // Data mode
    
    // 分批写入避免栈溢出
    const size_t chunk_size = 128;
    size_t remaining = len;
    const uint8_t *ptr = data;
    
    while (remaining > 0) {
        size_t to_write = (remaining > chunk_size) ? chunk_size : remaining;
        i2c_master_write(cmd, (uint8_t*)ptr, to_write, true);
        remaining -= to_write;
        ptr += to_write;
    }
    
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(SH1106_I2C_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

// I2C 总线初始化
static esp_err_t i2c_bus_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SH1106_I2C_SDA_PIN,
        .scl_io_num = SH1106_I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = SH1106_I2C_FREQ_HZ,
    };
    
    ESP_ERROR_CHECK(i2c_param_config(SH1106_I2C_NUM, &conf));
    return i2c_driver_install(SH1106_I2C_NUM, I2C_MODE_MASTER, 0, 0, 0);
}

// SH1106 初始化序列
esp_err_t sh1106_init(void) {
    ESP_LOGI(TAG, "初始化 SH1106 I2C 驱动...");
    
    // 设置默认字体
    current_font = (const uint8_t*)font_8x6;
    font_w = 6;
    font_h = 8;
    
    ESP_ERROR_CHECK(i2c_bus_init());
    vTaskDelay(pdMS_TO_TICKS(100));  // 等待上电稳定
    
    // 发送初始化命令序列
    const uint8_t init_cmds[] = {
        0xAE,           // Display OFF
        0xD5, 0x80,     // Set Display Clock Divide Ratio / Oscillator Frequency
        0xA8, 0x3F,     // Set Multiplex Ratio (64MUX)
        0xD3, 0x00,     // Set Display Offset
        0x40,           // Set Display Start Line
        0x8D, 0x14,     // Set Charge Pump (Enable)
        0x20, 0x00,     // Set Memory Addressing Mode (Horizontal)
        0xA1,           // Set Segment Re-map (左右镜像，根据实际调整)
        0xC8,           // Set COM Output Scan Direction (从下到上)
        0xDA, 0x12,     // Set COM Pins Hardware Configuration
        0x81, 0x80,     // Set Contrast Control
        0xD9, 0xF1,     // Set Pre-charge Period
        0xDB, 0x40,     // Set VCOMH Deselect Level
        0xA4,           // Set Entire Display ON (Resume RAM content)
        0xA6,           // Set Normal/Inverse Display (Normal)
        0xAF,           // Display ON
    };
    
    for (size_t i = 0; i < sizeof(init_cmds); i++) {
        ESP_ERROR_CHECK(sh1106_write_cmd(init_cmds[i]));
    }
    
    sh1106_clear();
    ESP_LOGI(TAG, "SH1106 初始化完成");
    return ESP_OK;
}

esp_err_t sh1106_deinit(void) {
    sh1106_display_off();
    i2c_driver_delete(SH1106_I2C_NUM);
    return ESP_OK;
}

// 显示控制
esp_err_t sh1106_display_on(void) {
    return sh1106_write_cmd(0xAF);
}

esp_err_t sh1106_display_off(void) {
    return sh1106_write_cmd(0xAE);
}

esp_err_t sh1106_set_contrast(uint8_t contrast) {
    ESP_ERROR_CHECK(sh1106_write_cmd(0x81));
    return sh1106_write_cmd(contrast);
}

esp_err_t sh1106_set_brightness(uint8_t brightness) {
    return sh1106_set_contrast(brightness);
}

esp_err_t sh1106_invert_display(bool invert) {
    return sh1106_write_cmd(invert ? 0xA7 : 0xA6);
}

// 屏幕翻转控制
esp_err_t sh1106_flip_display(bool horizontal, bool vertical) {
    ESP_ERROR_CHECK(sh1106_write_cmd(horizontal ? 0xA0 : 0xA1));  // Segment remap
    ESP_ERROR_CHECK(sh1106_write_cmd(vertical ? 0xC0 : 0xC8));    // COM scan direction
    return ESP_OK;
}

// 显存操作
esp_err_t sh1106_clear(void) {
    memset(sh1106_buffer, 0x00, sizeof(sh1106_buffer));
    memset(buffer_dirty, true, sizeof(buffer_dirty));
    full_refresh = true;
    return sh1106_refresh_full();
}

esp_err_t sh1106_fill(uint8_t pattern) {
    memset(sh1106_buffer, pattern, sizeof(sh1106_buffer));
    memset(buffer_dirty, true, sizeof(buffer_dirty));
    full_refresh = true;
    return ESP_OK;
}

// 刷新指定页到 OLED (SH1106 关键: 列偏移 +2)
static esp_err_t sh1106_refresh_page(uint8_t page) {
    if (page >= SH1106_PAGES) return ESP_ERR_INVALID_ARG;
    
    // 设置页地址
    ESP_ERROR_CHECK(sh1106_write_cmd(0xB0 + page));
    // 设置列地址 (低 4 位 + 偏移)
    ESP_ERROR_CHECK(sh1106_write_cmd(SH1106_OFFSET & 0x0F));
    // 设置列地址 (高 4 位)
    ESP_ERROR_CHECK(sh1106_write_cmd(0x10 | ((SH1106_OFFSET >> 4) & 0x0F)));
    
    // 写入 128 字节数据
    return sh1106_write_data_multi(sh1106_buffer[page], SH1106_WIDTH);
}

// 智能刷新: 只刷新脏页
esp_err_t sh1106_refresh(void) {
    if (full_refresh) {
        return sh1106_refresh_full();
    }
    
    for (uint8_t page = 0; page < SH1106_PAGES; page++) {
        if (buffer_dirty[page]) {
            ESP_ERROR_CHECK(sh1106_refresh_page(page));
            buffer_dirty[page] = false;
        }
    }
    return ESP_OK;
}

// 强制全屏刷新
esp_err_t sh1106_refresh_full(void) {
    for (uint8_t page = 0; page < SH1106_PAGES; page++) {
        ESP_ERROR_CHECK(sh1106_refresh_page(page));
        buffer_dirty[page] = false;
    }
    full_refresh = false;
    return ESP_OK;
}

// 像素绘制
esp_err_t sh1106_draw_pixel(int16_t x, int16_t y, uint8_t color) {
    if (x < 0 || x >= SH1106_WIDTH || y < 0 || y >= SH1106_HEIGHT) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t page = y >> 3;  // y / 8
    uint8_t bit = y & 0x07; // y % 8
    
    switch (color) {
        case SH1106_COLOR_BLACK:
            sh1106_buffer[page][x] &= ~(1 << bit);
            break;
        case SH1106_COLOR_WHITE:
            sh1106_buffer[page][x] |= (1 << bit);
            break;
        case SH1106_COLOR_INVERT:
            sh1106_buffer[page][x] ^= (1 << bit);
            break;
    }
    
    buffer_dirty[page] = true;
    return ESP_OK;
}

// Bresenham 画线算法
esp_err_t sh1106_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    
    while (true) {
        sh1106_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    return ESP_OK;
}

// 绘制矩形 (空心)
esp_err_t sh1106_draw_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t color) {
    sh1106_draw_line(x, y, x + w - 1, y, color);         // Top
    sh1106_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color); // Bottom
    sh1106_draw_line(x, y, x, y + h - 1, color);         // Left
    sh1106_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color); // Right
    return ESP_OK;
}

// 填充矩形 (实心)
esp_err_t sh1106_fill_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t color) {
    for (int16_t i = x; i < x + w && i < SH1106_WIDTH; i++) {
        for (int16_t j = y; j < y + h && j < SH1106_HEIGHT; j++) {
            sh1106_draw_pixel(i, j, color);
        }
    }
    return ESP_OK;
}

// 绘制圆形 (中点圆算法)
esp_err_t sh1106_draw_circle(int16_t x0, int16_t y0, uint16_t r, uint8_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    sh1106_draw_pixel(x0, y0 + r, color);
    sh1106_draw_pixel(x0, y0 - r, color);
    sh1106_draw_pixel(x0 + r, y0, color);
    sh1106_draw_pixel(x0 - r, y0, color);
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        sh1106_draw_pixel(x0 + x, y0 + y, color);
        sh1106_draw_pixel(x0 - x, y0 + y, color);
        sh1106_draw_pixel(x0 + x, y0 - y, color);
        sh1106_draw_pixel(x0 - x, y0 - y, color);
        sh1106_draw_pixel(x0 + y, y0 + x, color);
        sh1106_draw_pixel(x0 - y, y0 + x, color);
        sh1106_draw_pixel(x0 + y, y0 - x, color);
        sh1106_draw_pixel(x0 - y, y0 - x, color);
    }
    return ESP_OK;
}

// 绘制位图 (1-bit, MSB 在前)
esp_err_t sh1106_draw_bitmap(int16_t x, int16_t y, const uint8_t *bitmap, uint16_t w, uint16_t h) {
    for (uint16_t j = 0; j < h; j++) {
        for (uint16_t i = 0; i < w; i++) {
            uint16_t byte_idx = (j * w + i) >> 3;
            uint8_t bit_mask = 0x80 >> ((j * w + i) & 0x07);
            if (bitmap[byte_idx] & bit_mask) {
                sh1106_draw_pixel(x + i, y + j, SH1106_COLOR_WHITE);
            }
        }
    }
    return ESP_OK;
}

// 字体设置
void sh1106_set_font(const uint8_t *font_data, uint8_t font_width, uint8_t font_height) {
    current_font = font_data;
    font_w = font_width;
    font_h = font_height;
}

// 绘制字符 (使用当前字体)
esp_err_t sh1106_draw_char(int16_t x, int16_t y, char c, uint8_t color) {
    if (!current_font || c < 32 || c > 127) return ESP_ERR_INVALID_ARG;
    
    uint8_t idx = c - 32;
    // 修复: 删除未使用的变量 char_data
    // uint8_t bytes_per_char = (font_w * font_h + 7) >> 3;
    // const uint8_t *char_data = current_font + idx * bytes_per_char;
    
    // 简化的 8x6 字体处理
    if (font_w == 6 && font_h == 8 && current_font == (const uint8_t*)font_8x6) {
        for (uint8_t i = 0; i < 6; i++) {
            uint8_t line = font_8x6[idx][i];
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint8_t pixel_color = (line >> bit) & 1 ? color : SH1106_COLOR_BLACK;
                sh1106_draw_pixel(x + i, y + bit, pixel_color);
            }
        }
    }
    return ESP_OK;
}

// 绘制字符串
esp_err_t sh1106_draw_string(int16_t x, int16_t y, const char *str, uint8_t color) {
    int16_t orig_x = x;
    while (*str) {
        if (*str == '\n') {
            x = orig_x;
            y += font_h;
        } else {
            sh1106_draw_char(x, y, *str, color);
            x += font_w;
        }
        str++;
    }
    return ESP_OK;
}

// 获取文本边界框
void sh1106_get_text_bounds(const char *str, int16_t x, int16_t y, 
                            int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    uint16_t lines = 1, max_width = 0, cur_width = 0;
    for (const char *c = str; *c; c++) {
        if (*c == '\n') {
            lines++;
            if (cur_width > max_width) max_width = cur_width;
            cur_width = 0;
        } else {
            cur_width += font_w;
        }
    }
    if (cur_width > max_width) max_width = cur_width;
    
    *x1 = x;
    *y1 = y;
    *w = max_width;
    *h = lines * font_h;
}