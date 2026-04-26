#include "counter_mod.h"
#include "array_keypad.h"
#include "sh1106.h"

/* ==================== 栈的基本操作 ==================== */

void initValueStack(ValueStack *s) {
    s->top = -1;
}

void initOpStack(OpStack *s) {
    s->top = -1;
}

int isValueStackEmpty(ValueStack *s) {
    return s->top == -1;
}

int isOpStackEmpty(OpStack *s) {
    return s->top == -1;
}

void pushValue(ValueStack *s, double val) {
    if (s->top >= MAX_SIZE - 1) {
        printf("Error: Value stack overflow!\n");
        exit(1);
    }
    s->data[++(s->top)] = val;
}

void pushOp(OpStack *s, char op) {
    if (s->top >= MAX_SIZE - 1) {
        printf("Error: Operator stack overflow!\n");
        exit(1);
    }
    s->data[++(s->top)] = op;
}

double popValue(ValueStack *s) {
    if (isValueStackEmpty(s)) {
        printf("Error: Value stack underflow!\n");
        exit(1);
    }
    return s->data[(s->top)--];
}

char popOp(OpStack *s) {
    if (isOpStackEmpty(s)) {
        printf("Error: Operator stack underflow!\n");
        exit(1);
    }
    return s->data[(s->top)--];
}

char peekOp(OpStack *s) {
    if (isOpStackEmpty(s)) return '\0';
    return s->data[s->top];
}

/* ==================== 核心算法 ==================== */

// 获取运算符优先级
int precedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;  // '(' 或空
}

// 执行一次运算：弹出两个操作数和一个运算符，计算结果压回栈
void applyOp(ValueStack *values, OpStack *ops) {
    char op = popOp(ops);
    double b = popValue(values);  // 右操作数（先弹出）
    double a = popValue(values);  // 左操作数
    
    double result;
    switch (op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/': 
            if (b == 0) {
                printf("Error: Division by zero!\n");
                exit(1);
            }
            result = a / b; 
            break;
        default:
            printf("Error: Unknown operator %c\n", op);
            exit(1);
    }
    pushValue(values, result);
}

// 双栈法求值主函数
double evaluate(const char *expr) {
    ValueStack values;
    OpStack ops;
    initValueStack(&values);
    initOpStack(&ops);
    
    int i = 0;
    int len = strlen(expr);
    
    while (i < len) {
        unsigned char ch = expr[i];
        
        // 1. 跳过空格
        if (ch == ' ' || ch == '\t') {
            i++;
            continue;
        }
        
        // 2. 处理数字（支持整数和小数）
        if (isdigit(ch) || ch == '.') {
            double num = 0;
            // 整数部分
            while (i < len && isdigit((unsigned char)expr[i])) {
                num = num * 10 + (expr[i] - '0');
                i++;
            }
            // 小数部分
            if (i < len && expr[i] == '.') {
                i++;
                double factor = 0.1;
                while (i < len && isdigit((unsigned char)expr[i])) {
                    num += (expr[i] - '0') * factor;
                    factor *= 0.1;
                    i++;
                }
            }
            pushValue(&values, num);
            continue;  // i已经更新，跳过最后的i++
        }
        
        // 3. 处理左括号
        if (ch == '(') {
            pushOp(&ops, ch);
        }
        
        // 4. 处理右括号：计算到匹配的左括号
        else if (ch == ')') {
            while (!isOpStackEmpty(&ops) && peekOp(&ops) != '(') {
                applyOp(&values, &ops);
            }
            // 弹出左括号
            if (!isOpStackEmpty(&ops) && peekOp(&ops) == '(') {
                popOp(&ops);
            } else {
                printf("Error: Mismatched parentheses!\n");
                exit(1);
            }
        }
        
        // 5. 处理运算符 + - * /
        else if (ch == '+' || ch == '-' || ch == '*' || ch == '/') {
            // 当前运算符优先级 <= 栈顶优先级时，先计算栈顶的
            while (!isOpStackEmpty(&ops) && peekOp(&ops) != '(' &&
                   precedence(peekOp(&ops)) >= precedence(ch)) {
                applyOp(&values, &ops);
            }
            pushOp(&ops, ch);
        }
        
        else {
            printf("Error: Invalid character '%c'\n", ch);
            exit(1);
        }
        
        i++;
    }
    
    // 6. 处理剩余运算符
    while (!isOpStackEmpty(&ops)) {
        if (peekOp(&ops) == '(' || peekOp(&ops) == ')') {
            printf("Error: Mismatched parentheses!\n");
            exit(1);
        }
        applyOp(&values, &ops);
    }
    
    // 最终结果
    if (isValueStackEmpty(&values)) {
        return 0;
    }
    
    double finalResult = popValue(&values);
    // 检查是否还有剩余操作数（表达式错误）
    if (!isValueStackEmpty(&values)) {
        printf("Error: Invalid expression!\n");
        exit(1);
    }
    
    return finalResult;
}
