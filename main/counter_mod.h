#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_SIZE 100

/* ==================== 栈结构定义 ==================== */

// 操作数栈（double类型，支持小数）
typedef struct {
    double data[MAX_SIZE];
    int top;
} ValueStack;

// 运算符栈
typedef struct {
    char data[MAX_SIZE];
    int top;
} OpStack;

/* ==================== 栈的基本操作 ==================== */

void initValueStack(ValueStack *s);
void initOpStack(OpStack *s);
int isValueStackEmpty(ValueStack *s);
int isOpStackEmpty(OpStack *s);
void pushValue(ValueStack *s, double val);
void pushOp(OpStack *s, char op);
double popValue(ValueStack *s);
char popOp(OpStack *s);
char peekOp(OpStack *s);

/* ==================== 核心算法 ==================== */

int precedence(char op);
void applyOp(ValueStack *values, OpStack *ops);
double evaluate(const char *expr);

void test_work_cal(void *pvParameters);