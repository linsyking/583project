typedef struct
{
    char c[3];
} type;

int func()
{
    type a[200];
    type b[2];
    return (long int)a + (long int)b;
}
