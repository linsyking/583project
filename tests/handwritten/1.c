int c[1024];

void func(int* a, int *b, int* m) {
    for ( int i = 0; i < 1024; i++ ) {
        if ( m[i] < 0 ) {
            c[i] = a[i] + b[i];
        }
        else {
            c[i] = 0;
        }
    }
}

int main() {
    int a[10], b[10], m[1024];
    func(a, b, m);
}
