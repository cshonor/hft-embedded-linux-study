/* lstat-based attribute dump (tiny ls-style).
 * cc -Wall -Wextra -o print_stat print_stat.c && ./print_stat PATH...
 */
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static const char *type_str(mode_t m)
{
    if (S_ISREG(m))  return "reg";
    if (S_ISDIR(m))  return "dir";
    if (S_ISLNK(m))  return "lnk";
    if (S_ISCHR(m))  return "chr";
    if (S_ISBLK(m))  return "blk";
    if (S_ISFIFO(m)) return "fifo";
    if (S_ISSOCK(m)) return "sock";
    return "?";
}

static void mode_str(mode_t m, char out[11])
{
    out[0] = S_ISDIR(m) ? 'd' : S_ISLNK(m) ? 'l' : S_ISCHR(m) ? 'c' :
             S_ISBLK(m) ? 'b' : S_ISFIFO(m) ? 'p' : S_ISSOCK(m) ? 's' : '-';
    out[1] = (m & S_IRUSR) ? 'r' : '-';
    out[2] = (m & S_IWUSR) ? 'w' : '-';
    out[3] = (m & S_IXUSR) ? ((m & S_ISUID) ? 's' : 'x') : ((m & S_ISUID) ? 'S' : '-');
    out[4] = (m & S_IRGRP) ? 'r' : '-';
    out[5] = (m & S_IWGRP) ? 'w' : '-';
    out[6] = (m & S_IXGRP) ? ((m & S_ISGID) ? 's' : 'x') : ((m & S_ISGID) ? 'S' : '-');
    out[7] = (m & S_IROTH) ? 'r' : '-';
    out[8] = (m & S_IWOTH) ? 'w' : '-';
    out[9] = (m & S_IXOTH) ? ((m & S_ISVTX) ? 't' : 'x') : ((m & S_ISVTX) ? 'T' : '-');
    out[10] = '\0';
}

static void print_one(const char *path)
{
    struct stat st;
    char perms[11];
    struct passwd *pw;
    struct group *gr;
    char tbuf[64];

    if (lstat(path, &st) == -1) {
        perror(path);
        return;
    }

    mode_str(st.st_mode, perms);
    pw = getpwuid(st.st_uid);
    gr = getgrgid(st.st_gid);

    printf("%s\n", path);
    printf("  type=%s  mode=%s (%04o)  nlink=%lu\n",
           type_str(st.st_mode), perms,
           (unsigned)(st.st_mode & 07777),
           (unsigned long)st.st_nlink);
    printf("  dev=%llu ino=%llu  uid=%u(%s) gid=%u(%s)\n",
           (unsigned long long)st.st_dev,
           (unsigned long long)st.st_ino,
           (unsigned)st.st_uid, pw ? pw->pw_name : "?",
           (unsigned)st.st_gid, gr ? gr->gr_name : "?");
    printf("  size=%lld  blksize=%ld  blocks=%lld\n",
           (long long)st.st_size, (long)st.st_blksize,
           (long long)st.st_blocks);

    strftime(tbuf, sizeof(tbuf), "%F %T", localtime(&st.st_atim.tv_sec));
    printf("  atime %s\n", tbuf);
    strftime(tbuf, sizeof(tbuf), "%F %T", localtime(&st.st_mtim.tv_sec));
    printf("  mtime %s\n", tbuf);
    strftime(tbuf, sizeof(tbuf), "%F %T", localtime(&st.st_ctim.tv_sec));
    printf("  ctime %s  (inode change; not birth)\n", tbuf);
}

int main(int argc, char *argv[])
{
    int i;
    if (argc < 2) {
        fprintf(stderr, "usage: %s PATH...\n", argv[0]);
        return EXIT_FAILURE;
    }
    for (i = 1; i < argc; i++)
        print_one(argv[i]);
    return 0;
}
