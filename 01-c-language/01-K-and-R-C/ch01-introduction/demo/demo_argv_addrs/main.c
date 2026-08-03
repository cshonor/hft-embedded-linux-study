#include <stdio.h>

/*
 * Three different addresses:
 *   &argv[i]  — slot in the pointer table (char **)
 *   argv[i]   — value of that slot: start of the C string (char *)
 *   &argv[i][0] — same as argv[i] for a non-empty string
 */
int main(int argc, char *argv[])
{
    int i;

    printf("argv (decayed)     = %p\n", (void *)argv);
    printf("&argv[0]           = %p  (first table slot)\n", (void *)&argv[0]);
    printf("sizeof(argv[0])    = %zu  (one char *)\n", sizeof(argv[0]));

    for (i = 0; i < argc; i++) {
        printf("argv[%d]: slot=%p  str=%p  text=\"%s\"\n",
               i,
               (void *)&argv[i],
               (void *)argv[i],
               argv[i]);
    }
    printf("argv[%d] sentinel  = %p (expect NULL)\n",
           argc, (void *)argv[argc]);
    return 0;
}
