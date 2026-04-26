#include "task_list.h"
#include "sh1106.h"
#include "array_keypad.h"
#include "counter_mod.h"

static const char *TAG = "all_of_tasks";

//创建队列
QueueHandle_t q_keypad_to_calc;
QueueHandle_t q_calc_to_disp;

void task_system_init(void) {
    q_keypad_to_calc = xQueueCreate(10, sizeof(key_event_t));
    q_calc_to_disp   = xQueueCreate(5,  sizeof(disp_msg_t));

    // 优先级：Display > Keypad > Calculator
    // Display需要及时刷新避免撕裂感
    xTaskCreate(task_keypad,     "key",  2048, NULL, 2, NULL);
    xTaskCreate(task_calculator, "calc", 4096, NULL, 2, NULL);
    xTaskCreate(task_display,    "disp", 2048, NULL, 3, NULL);
}

// ========== 主任务 ==========
void task_keypad(void *pvParameters)
{
    key_event_t evt;
    keypad_init();
    ESP_LOGI(TAG, "Keypad initialized on GPIO 10-17");

    while (1) {
        char key = keypad_scan();  // 你的矩阵扫描函数
        if (key != 0) {
            evt.value = key;
            // 根据key判断type
            if ((key >= '0' && key <= '9') || key == '.') {
                evt.type = KEY_NUM;
            }
            else if (strchr("+-*/", key)) {
                evt.type = KEY_OP;
            }
            else {
                evt.type = KEY_FUNC;
            }
            
            xQueueSend(q_keypad_to_calc, &evt, portMAX_DELAY);
        }
        vTaskDelay(pdMS_TO_TICKS(50));  // 20Hz扫描，消抖
    }
}

void task_calculator(void *pvParameters) {
    key_event_t evt;
    disp_msg_t disp;
    char expr[64] = {0};   // 当前表达式缓冲区
    int expr_len = 0;
    
    while (1) {
        if (xQueueReceive(q_keypad_to_calc, &evt, portMAX_DELAY)) {
            
            switch (evt.type) {
                case KEY_NUM:
                case KEY_OP:
                    if (expr_len < 63) {
                        expr[expr_len++] = evt.value;
                        expr[expr_len] = '\0';
                        // 实时回显表达式
                        disp.cmd = DISP_EXPR;
                        strcpy(disp.text, expr);
                        xQueueSend(q_calc_to_disp, &disp, portMAX_DELAY);
                    }
                    break;
                    
                case KEY_FUNC:
                    if (evt.value == '=') {
                        // 调用你的双栈法计算
                        double result = evaluate(expr);
                        disp.cmd = DISP_RESULT;
                        snprintf(disp.text, 32, "%.6g", result);
                        xQueueSend(q_calc_to_disp, &disp, portMAX_DELAY);
                        expr_len = 0;  // 计算完清空，准备下一次
                    }
                    else if (evt.value == 'C') {
                        expr_len = 0;
                        expr[0] = '\0';
                        disp.cmd = DISP_CLEAR;
                        xQueueSend(q_calc_to_disp, &disp, portMAX_DELAY);
                    }
                    else if (evt.value == 'D') {  // DEL退格
                        if (expr_len > 0) expr[--expr_len] = '\0';
                        disp.cmd = DISP_EXPR;
                        strcpy(disp.text, expr);
                        xQueueSend(q_calc_to_disp, &disp, portMAX_DELAY);
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

void task_display(void *pvParameters) {
    disp_msg_t msg;
    while (1) {
        if (xQueueReceive(q_calc_to_disp, &msg, portMAX_DELAY)) {
            switch (msg.cmd) {
                case DISP_EXPR:
                    sh1106_clear();
                    sh1106_draw_string(0, 0, msg.text, SH1106_COLOR_WHITE);
                    sh1106_refresh();
                    break;
                case DISP_RESULT:
                    sh1106_draw_string(0, 10, msg.text, SH1106_COLOR_WHITE);
                    sh1106_refresh();
                    break;
                case DISP_CLEAR:
                    sh1106_clear();
                    sh1106_refresh();
                    break;
                case DISP_ERROR:    // ← 添加
                    sh1106_draw_string(0, 20, "Error", SH1106_COLOR_WHITE);
                    sh1106_refresh();
                    break;
                default:
                    break;
            }
        }
    }
}