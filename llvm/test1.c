/* Module 8 sample A — simple arithmetic and memory (for clang -emit-llvm). */
#include <stdio.h>

int main(void) {
    int a = 3 + 4;
    int b = a * 2;
    printf("%d\n", b);
    return 0;
}
