#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "task_list.h"
#include "sh1106.h"
#include "array_keypad.h"
#include "counter_mod.h"

#include "esp_log.h"

void app_main(void)
{

    // 初始化 SH1106
    ESP_ERROR_CHECK(sh1106_init());
    sh1106_clear();
    sh1106_draw_string(0, 0, "ESP32-S3 + SH1106", SH1106_COLOR_WHITE);
    sh1106_draw_string(0, 10, "1.3 inch OLED", SH1106_COLOR_WHITE);
    sh1106_draw_string(0, 20, "128x64 I2C", SH1106_COLOR_WHITE);
    sh1106_refresh();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 优先级：Display > Keypad > Calculator
    // Display需要及时刷新避免撕裂感
    task_system_init();  // 在这里初始化
}