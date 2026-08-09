#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(void)
{
    long pagesz;
    size_t len;
    char *p;
    unsigned char *vec;
    size_t npages;

    pagesz = sysconf(_SC_PAGESIZE);
    if (pagesz < 0) {
        perror("sysconf");
        return 1;
    }
    len = (size_t)pagesz * 2;

    p = mmap(NULL, len, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    strcpy(p, "writable");
    printf("wrote: %s\n", p);

    if (mprotect(p, (size_t)pagesz, PROT_READ) == -1) {
        perror("mprotect READ");
        return 1;
    }
    printf("page0 now PROT_READ (write would SIGSEGV)\n");

    npages = len / (size_t)pagesz;
    vec = calloc(npages, 1);
    if (vec == NULL) {
        perror("calloc");
        return 1;
    }
    if (mincore(p, len, vec) == -1) {
        perror("mincore");
        return 1;
    }
    printf("mincore: page0 resident=%d page1 resident=%d (snapshot)\n",
           vec[0] & 1, vec[1] & 1);

    if (madvise(p, len, MADV_WILLNEED) == -1)
        perror("madvise WILLNEED");
    else
        printf("madvise(MADV_WILLNEED) ok\n");

    if (mlock(p, (size_t)pagesz) == -1)
        perror("mlock (often fails if ulimit -l too small)");
    else {
        printf("mlock ok\n");
        munlock(p, (size_t)pagesz);
    }

    free(vec);
    munmap(p, len);
    return 0;
}
