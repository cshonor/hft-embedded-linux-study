/* Listing 7-1 spirit: free() of small blocks often does not lower program break. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_ALLOCS 1000

int main(int argc, char *argv[]) {
    int num = (argc > 1) ? atoi(argv[1]) : 100;
    int block = (argc > 2) ? atoi(argv[2]) : 1024;
    int free_step = (argc > 3) ? atoi(argv[3]) : 1;
    int free_min = (argc > 4) ? atoi(argv[4]) : 1;
    int free_max = (argc > 5) ? atoi(argv[5]) : num;

    if (num > MAX_ALLOCS)
        num = MAX_ALLOCS;

    char *ptr[MAX_ALLOCS];
    printf("initial program break:           %p\n", sbrk(0));

    printf("allocating %d*%d bytes\n", num, block);
    for (int i = 0; i < num; i++) {
        ptr[i] = malloc((size_t)block);
        if (ptr[i] == NULL) {
            perror("malloc");
            return 1;
        }
    }
    printf("program break after malloc:      %p\n", sbrk(0));

    printf("freeing blocks from %d to %d in steps of %d\n",
           free_min, free_max, free_step);
    for (int j = free_min - 1; j < free_max; j += free_step)
        free(ptr[j]);

    printf("program break after free():      %p\n", sbrk(0));
    printf("(often unchanged for small blocks — glibc freelist cache)\n");
    return 0;
}
