#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// 推荐方案：GPIO 10-17（功能单纯，支持 RTC 唤醒）
#define ROW_0    GPIO_NUM_10   // 输出行
#define ROW_1    GPIO_NUM_11
#define ROW_2    GPIO_NUM_12
#define ROW_3    GPIO_NUM_13

#define COL_0    GPIO_NUM_14   // 输入列（内部上拉）
#define COL_1    GPIO_NUM_15
#define COL_2    GPIO_NUM_16
#define COL_3    GPIO_NUM_17

// ========== 引脚配置 ==========
// 行（输出）
static const gpio_num_t row_pins[4] = {
    GPIO_NUM_10, GPIO_NUM_11, GPIO_NUM_12, GPIO_NUM_13
};
// 列（输入，内部上拉）
static const gpio_num_t col_pins[4] = {
    GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17
};

// 按键映射表
static const char key_map[4][4] = {
    {'1', '2', '3', '+'},
    {'4', '5', '6', '-'},
    {'7', '8', '9', '*'},
    {'#', '0', '=', '/'}
};

typedef enum {
    KEY_NUM,        // 数字0-9
    KEY_OP,         // 运算符 + - * /
    KEY_FUNC,       // 功能键 = C DEL
    KEY_NONE
} key_type_t;

typedef struct {
    key_type_t type;
    char value;     // 具体字符，如 '7', '+', '=', 'C'
} key_event_t;

// 显示指令类型
typedef enum {
    DISP_EXPR,      // 显示表达式
    DISP_RESULT,    // 显示结果
    DISP_CLEAR,     // 清屏
    DISP_ERROR      // 显示错误
} disp_cmd_t;

typedef struct {
    disp_cmd_t cmd;
    char text[32];  // 要显示的字符串
} disp_msg_t;

void keypad_init(void);
char keypad_scan(void);
char keypad_get_key(void);