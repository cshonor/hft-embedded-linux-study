/* Simple directory listing with lstat types.
 * cc -Wall -Wextra -o list_dir list_dir.c && ./list_dir [DIR]
 */
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *type_of(const char *dir, const char *name)
{
    char path[4096];
    struct stat st;

    if (snprintf(path, sizeof(path), "%s/%s", dir, name) >= (int)sizeof(path))
        return "?";
    if (lstat(path, &st) == -1)
        return "?";
    if (S_ISREG(st.st_mode))  return "reg";
    if (S_ISDIR(st.st_mode))  return "dir";
    if (S_ISLNK(st.st_mode))  return "lnk";
    if (S_ISFIFO(st.st_mode)) return "fifo";
    if (S_ISSOCK(st.st_mode)) return "sock";
    if (S_ISCHR(st.st_mode))  return "chr";
    if (S_ISBLK(st.st_mode))  return "blk";
    return "?";
}

int main(int argc, char *argv[])
{
    const char *dir = (argc > 1) ? argv[1] : ".";
    DIR *dp;
    struct dirent *de;

    dp = opendir(dir);
    if (dp == NULL) {
        perror(dir);
        return EXIT_FAILURE;
    }

    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        printf("%-6s %s\n", type_of(dir, de->d_name), de->d_name);
    }
    if (errno != 0)
        perror("readdir");
    closedir(dp);
    return 0;
}
