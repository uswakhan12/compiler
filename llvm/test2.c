/* Module 8 sample B — loop and conditional for optimisation comparison. */
#include <stdio.h>

int main(void) {
    int s = 0;
    for (int i = 0; i < 10000; i++) {
        s += i;
        if (s > 50000000)
            break;
    }
    printf("%d\n", s);
    return 0;
}
