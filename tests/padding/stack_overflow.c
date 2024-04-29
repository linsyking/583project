#include <stdio.h>

int i = 0;

int func()
{
    int a[4];
    printf("%d: %d %d\n", i++, (long int)a, a[7]);
    func();
    return 0;
}

int main()
{
    func();
    return 0;
}