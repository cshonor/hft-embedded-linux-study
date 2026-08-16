#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

int main(void)
{
    char line[32];
    FILE *fp = fopen("/nonexistent/path/for_demo.txt", "r");

    if (!fp) {
        puts("fopen failed (expected), fp=NULL — must not fread");
    }

    /* fmemopen: read "hello\nworld" from memory */
    fp = fmemopen((void *)"hello\nworld", 11, "r");
    if (!fp) {
        perror("fmemopen");
        return 1;
    }

    if (fgets(line, sizeof(line), fp)) {
        printf("raw fgets: [%s] len=%zu\n", line, strlen(line));
        line[strcspn(line, "\n")] = '\0';
        printf("strip \\n:   [%s]\n", line);
    }

    fclose(fp);
    return 0;
}
