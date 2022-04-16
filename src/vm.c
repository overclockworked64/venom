#include <stdio.h>
#include "compiler.h"
#include "vm.h"

void push(int value) {
    stack[tos++] = value;
}

int pop() {
    return stack[--tos];
}

void run() {
    tos = 0;
    for (char *i = compiling_chunk.ip; i < &compiling_chunk.ip[compiling_chunk.count]; ++i) {
        switch (*i) {
            case OP_PRINT: {
                int value = pop();
                printf("%d\n", value);
                break;
            }
            case OP_CONST: {
                push(*++i);
                break;
            }
            case OP_ADD: {
                int b = pop();
                int a = pop();
                push(a + b);
                break;
            }
            case OP_SUB: {
                int b = pop();
                int a = pop();
                push(a - b);
                break;
            }
            case OP_MUL: {
                int b = pop();
                int a = pop();
                push(a * b);
                break;
            }
            case OP_DIV: {
                int b = pop();
                int a = pop();
                push(a / b);
                break;
            }
            default:
                break;
        }
    }
}