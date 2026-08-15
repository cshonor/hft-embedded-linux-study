# TLPI 第 18 章 — Directories and Links

**优先级**：🔴（路径树操作、临时文件、TOCTOU、可靠写入）  
**前置**：[Ch17 ACL](../chapter-17-access-control-lists/README.md) · [Ch14 FS/inode](../chapter-14-file-systems/README.md) · [Ch15 stat/lstat](../chapter-15-file-attributes/README.md)  
**后置**：[Ch19 inotify](../chapter-19-monitoring-file-events/README.md)

---

## 小节目录

- [18.1 目录基础](notes/18.1-directories-and-hard-links.md)
- [18.2 读目录](notes/18.10-the-current-working-directory-of-a-proce.md)
- [18.3 硬链接](notes/18.3-creating-and-removing-hard-links-link-an.md)
- [18.4 符号链接](notes/18.3-creating-and-removing-hard-links-link-an.md)
- [18.5 `rename`（同 FS 原子）](notes/18.4-changing-the-name-of-a-file-rename.md)
- [18.6 路径与 TOCTOU](notes/18.6-creating-and-removing-directories-mkdir-.md)

---

## 章节目标


理清目录 / 硬链 / 软链；掌握 mkdir/rmdir、目录遍历、link/symlink/unlink、`rename` 原子性；用 `*at` + flags 缓解路径竞态。

---


---

## 18.7 易错清单


1. 目录要访问内容：常需 `x`，不只 `r`  
2. 硬链禁目录；软链可以  
3. `unlink` 减计数，不是立刻抹数据  
4. `rename` 原子性限于**同一文件系统**  
5. `readlink` ≠ 解析最终路径  
6. 硬链不新建数据 inode；软链新建 inode  
7. 无「递归 rmdir」syscall；自行遍历 unlink + rmdir  
8. 根 `..` 指自己，遍历防环  

---


---

## 练习


1. `opendir`/`readdir` + `lstat` 区分类型  
2. `link` vs `symlink`，比 `st_ino`  
3. 临时文件 + `rename` 安全写  
4. open 后 unlink 仍可读  
5. 目录仅 `r` 无 `x` 的行为  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 目录项存名字→inode；权限 r/x/w 语义不同 |
| 2 | 硬链共享 inode，同 FS，不链目录 |
| 3 | 软链独立 inode，可跨 FS / 悬空 |
| 4 | `nlink==0` 且无 fd → 才释放 |
| 5 | `rename` 同 FS 原子；可靠写 = write tmp + rename |
| 6 | 用 `*at` + `AT_SYMLINK_NOFOLLOW` 减 TOCTOU |

---


---

## 参考


- Kerrisk · TLPI Ch18  
- `man 2 mkdir` · `man 3 opendir` · `man 2 link` · `man 2 symlink` · `man 2 rename` · `man 2 unlink`


---

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>

/* Ch18 目录与链接 — mkdir/symlink/readlink/rename/link。
 * 演示硬链接/符号链接的区别。
 * 编译: gcc -o ch18_demo ch18_demo.c */

int main(void) {
    /* 创建目录 */
    mkdir("/tmp/ch18_dir", 0755);

    /* 创建文件并硬链接 */
    int fd = open("/tmp/ch18_dir/file.txt", O_CREAT | O_WRONLY, 0644);
    write(fd, "hello\n", 6);
    close(fd);

    link("/tmp/ch18_dir/file.txt", "/tmp/ch18_dir/hardlink.txt");
    printf("hard link created (same inode)\n");

    /* 符号链接 */
    symlink("/tmp/ch18_dir/file.txt", "/tmp/ch18_dir/softlink.txt");

    /* readlink 读取符号链接目标 */
    char buf[256];
    ssize_t n = readlink("/tmp/ch18_dir/softlink.txt", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("symlink target: %s\n", buf);
    }

    /* stat vs lstat: lstat 不跟随符号链接 */
    struct stat sb1, sb2;
    stat("/tmp/ch18_dir/softlink.txt", &sb1);
    lstat("/tmp/ch18_dir/softlink.txt", &sb2);
    printf("stat inode:  %lu (target file)\n", (unsigned long)sb1.st_ino);
    printf("lstat inode: %lu (link itself)\n", (unsigned long)sb2.st_ino);

    /* rename */
    rename("/tmp/ch18_dir/hardlink.txt", "/tmp/ch18_dir/renamed.txt");
    printf("renamed hardlink\n");

    /* 清理 */
    remove("/tmp/ch18_dir/softlink.txt");
    remove("/tmp/ch18_dir/renamed.txt");
    remove("/tmp/ch18_dir/file.txt");
    rmdir("/tmp/ch18_dir");
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
