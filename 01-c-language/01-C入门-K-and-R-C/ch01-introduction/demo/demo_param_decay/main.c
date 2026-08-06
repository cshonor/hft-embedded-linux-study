#include <stdio.h>

static void func(int arr[])
{
    /* Parameter already rewritten as int * — not an array object. */
    printf("sizeof(arr)  = %zu  (pointer width)\n", sizeof(arr));
}

int main(void)
{
    int buf[10];

    printf("sizeof(buf)  = %zu  (whole array)\n", sizeof(buf));
    printf("sizeof(&buf) = %zu  (pointer-to-array)\n", sizeof(&buf));
    printf("a vs &a types differ; +1 stride differs — see note 5.3.1\n");
    func(buf);
    return 0;
}
