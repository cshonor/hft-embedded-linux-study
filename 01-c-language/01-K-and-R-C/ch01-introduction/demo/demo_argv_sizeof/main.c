#include <stdio.h>

static void show_param(char *arr[])
{
    /* First [] in a parameter decays: arr is char **, not an array object. */
    printf("param sizeof(arr)=%zu  (expect sizeof(char **))\n", sizeof(arr));
    printf("param arr=%p  arr[0]=%s\n", (void *)arr, arr[0]);
}

int main(int argc, char *argv[])
{
    char *local[] = { "x", "y", NULL }; /* real array; length from initializer */

    (void)argc;
    printf("main  sizeof(argv)=%zu  (pointer, not array)\n", sizeof(argv));
    printf("local sizeof(local)=%zu  (3 * sizeof(char *))\n", sizeof(local));
    show_param(argv);
    show_param(local);

    /* Uncommenting the next line fails to compile (incomplete array type): */
    /* char *bad[]; */
    return 0;
}
