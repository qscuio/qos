#include <stdio.h>
#include <setjmp.h>

// 1. setjmp(buf) 的作用
// setjmp 用来保存当前的执行环境，包括：
//
// 程序的栈状态
// 寄存器内容
// 程序计数器（PC）
// 调用 setjmp(buf) 时会返回一个值：
//
// 如果直接调用，返回 0。
// 如果通过 longjmp 跳回来，返回 longjmp 提供的值。
// 2. longjmp(buf, value) 的作用
// longjmp 用来跳转到之前保存的环境（通过 setjmp 保存的点）。
// longjmp 会让 setjmp 返回 value。
// 通过这种机制，可以从程序深层直接跳回特定点，并提供一个返回值来区分是直接返回还是跳回的。
//
jmp_buf buf;  // 定义保存环境的全局变量

void second() {
    printf("In second() function\n");
    longjmp(buf, 1);  // 跳回到 setjmp 调用的地方，并返回 1
}

void first() {
    printf("In first() function\n");
    second();  // 调用 second()，它会使用 longjmp
    printf("This line will never execute\n");  // 不会执行，因为被 longjmp 跳过
}

int main() {
    if (setjmp(buf)) {  // 保存环境点
        // 从 longjmp 返回到这里
        printf("Back in main() after longjmp\n");
    } else {
        printf("Calling first() function\n");
        first();  // 调用 first()
    }

    printf("Program continues here after longjmp\n");
    return 0;
}

