/* Portable-ish df via statvfs.
 * cc -Wall -Wextra -o mini_df mini_df.c && ./mini_df [/path ...]
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>

static void print_one(const char *path)
{
    struct statvfs s;
    unsigned long long bsize, total, freeb, avail, used;
    double pct;

    if (statvfs(path, &s) == -1) {
        perror(path);
        return;
    }

    /* Prefer f_frsize (fundamental) when non-zero */
    bsize = s.f_frsize ? s.f_frsize : s.f_bsize;
    total = (unsigned long long)s.f_blocks * bsize;
    freeb = (unsigned long long)s.f_bfree * bsize;
    avail = (unsigned long long)s.f_bavail * bsize;
    used = total - freeb;
    pct = total ? (100.0 * (double)used / (double)total) : 0.0;

    printf("%s\n", path);
    printf("  block size (frsize): %llu\n", (unsigned long long)bsize);
    printf("  total bytes: %llu\n", total);
    printf("  used  bytes: %llu (%.1f%%)\n", used, pct);
    printf("  avail bytes: %llu (non-root)\n", avail);
    printf("  inodes: total=%llu free=%llu\n",
           (unsigned long long)s.f_files,
           (unsigned long long)s.f_ffree);
}

int main(int argc, char *argv[])
{
    int i;

    if (argc < 2) {
        print_one(".");
        return 0;
    }
    for (i = 1; i < argc; i++)
        print_one(argv[i]);
    return 0;
}
