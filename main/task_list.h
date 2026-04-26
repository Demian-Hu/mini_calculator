#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void task_system_init(void);
void task_keypad(void *pvParameters);
void task_calculator(void *pvParameters);
void task_display(void *pvParameters);