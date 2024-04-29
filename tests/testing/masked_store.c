#include <stdio.h>
int c[1024] = {1, 1, 1, 1, 1, 1, 1, 1};

void func(int *a, int *m) {
#pragma nounroll
    for (int i = 0; i < 1024; i++) {
        if (m[i] < 0) {
            // a[i] = 0;
        } else {
            c[i] = 1;
        }
    }
}

int main() {
    int a[16]   = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1};
    int m[1024] = {};

#pragma nounroll
    for (int i = 0; i < 8; ++i) {
        m[i] = -3;
    }
    func(a, m);
#pragma nounroll
    for (int i = 0; i < 16; i++) {
        printf("c %d\n", c[i]);
    }
#pragma nounroll
    for (int i = 0; i < 16; i++) {
        printf("a %d\n", a[i]);
    }
}
