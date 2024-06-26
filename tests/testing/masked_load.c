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
    int a[16]  = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1};
    int b[16]  = {3, 4, 5, 6, 6, 7, 8, 3, 9, 1, 1};
    int m[1024] = {};
    // a[10] = 0;
    // #pragma nounroll
    for (int i = 0; i < 12; ++i) {
        m[i] = -1;
    }
    func(a, b, m);
#pragma nounroll
    for (int i = 0; i < 16; i++) {
        printf("%d\n", c[i]);
    }
}
