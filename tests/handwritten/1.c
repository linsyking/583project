#include <stdio.h>
int c[1024] = {};

void func(int *a, int *b, int *m) {
#pragma nounroll
    for (int i = 0; i < 1024; i++) {
        if (m[i] < 0) {
            c[i] = a[i] + b[i];
        } else {
            c[i] = 1;
        }
    }
}

int main() {
    int a[12]  = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
    int b[12]  = {3, 4, 5, 6, 6, 7, 8, 3, 9, 1};
    int m[1024] = {};
    // #pragma nounroll
    for (int i = 0; i < 10; ++i) {
        m[i] = -1;
    }
    func(a, b, m);
#pragma nounroll
    for (int i = 0; i < 15; i++) {
        printf("%d\n", c[i]);
    }
}
