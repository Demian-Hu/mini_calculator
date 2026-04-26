#include "array_keypad.h"

// ========== 初始化 ==========
void keypad_init(void)
{
    gpio_config_t io_conf = {};

    // 配置行引脚为输出，初始低电平
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 0;
    for (int i = 0; i < 4; i++) {
        io_conf.pin_bit_mask |= (1ULL << row_pins[i]);
    }
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // 所有行初始设为高电平（或低电平，取决于扫描逻辑）
    for (int i = 0; i < 4; i++) {
        gpio_set_level(row_pins[i], 1);  // 默认高
    }

    // 配置列引脚为输入，启用内部上拉
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 0;
    for (int i = 0; i < 4; i++) {
        io_conf.pin_bit_mask |= (1ULL << col_pins[i]);
    }
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;  // 关键：内部上拉
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);
}

// ========== 扫描按键（逐行扫描法） ==========
char keypad_scan(void)
{
    for (int row = 0; row < 4; row++) {
        // 当前行输出低电平，其他行高电平
        for (int r = 0; r < 4; r++) {
            gpio_set_level(row_pins[r], (r == row) ? 0 : 1);
        }

        // 延时等待电平稳定（消抖）
        esp_rom_delay_us(10);

        // 读取所有列
        for (int col = 0; col < 4; col++) {
            if (gpio_get_level(col_pins[col]) == 0) {  // 低电平表示按下
                // 等待按键释放（简单消抖）
                while (gpio_get_level(col_pins[col]) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                return key_map[row][col];
            }
        }
    }
    return 0;  // 无按键
}

// ========== 带消抖的按键检测 ==========
char keypad_get_key(void)
{
    static uint32_t last_press_time = 0;
    const uint32_t debounce_ms = 200;  // 消抖间隔

    char key = keypad_scan();
    if (key != 0) {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (now - last_press_time > debounce_ms) {
            last_press_time = now;
            return key;
        }
    }
    return 0;
}

