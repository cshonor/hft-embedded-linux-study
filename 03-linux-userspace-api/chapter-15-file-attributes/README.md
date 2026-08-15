# TLPI 第 15 章 — File Attributes

**优先级**：🔴（权限 / 安全 / 目录与链接编程基础）  
**前置**：[Ch14 File Systems](../chapter-14-file-systems/README.md)（inode 模型）  
**后置**：[Ch16 Extended Attributes](../chapter-16-extended-attributes/README.md) · [Ch18 目录与链接](../chapter-18-directories-links/README.md) · [Ch38 特权与安全](../chapter-38-secure-privileged/README.md)

---

## 小节目录

- [15.1 `stat` / `lstat` / `fstat`](notes/15.1-retrieving-file-information-stat.md)
- [15.2 `st_mode`：类型与权限](notes/15.2-file-timestamps.md)
- [15.3 `chmod` / `fchmod`](notes/15.3-file-ownership.md)
- 15.4 `chown` / `fchown` / `lchown`
- 15.5 atime / mtime / ctime
- [15.6 `umask`](notes/15.6-summary.md)
- [15.7 `access()` — 尽量别用](notes/15.7-exercises.md)
- 15.9 `statx`（Linux）

---

## 章节目标


掌握 `stat` 系列读 inode；解析类型与权限；会改权限/属主/时间戳；理解 SUID/SGID/粘滞位与 atime/mtime/ctime；理清接口权限约束；警惕 `access()`。

---


---

## 15.8 速查：`stat` 族 vs `statx` · 时间接口


| API | 可移植 | 链接 | 备注 |
|-----|--------|------|------|
| `stat` | ✅ | 跟随 | 经典 |
| `lstat` | ✅ | 不跟随 | 看链接本身 |
| `fstat` | ✅ | — | 推荐：已有 fd |
| `statx` | Linux 4.11+ | `AT_SYMLINK_NOFOLLOW` 等 | 纳秒、btime、按字段 mask |

| 时间接口 | 精度 | 备注 |
|----------|------|------|
| `utime` | 秒 | 旧 |
| `utimes` | 微秒 | |
| `futimens` / `utimensat` | 纳秒 | **SUSv4 推荐** |

---


---

## 15.10 易错清单


1. `stat` 跟随；`lstat` 才见 `S_ISLNK`  
2. ctime ≠ 创建时间；改元数据会动 ctime  
3. 不能手动设 ctime  
4. sticky 对**目录**有意义  
5. 改时间用 `futimens`/`utimensat`，不是 `chmod`  
6. 避开 `access()`（RUID vs EUID + TOCTOU）  
7. `st_ino` 仅同 FS 可比  
8. 创建 mode 被 umask 裁剪  

---


---

## 练习


1. 简易 `ls`：`lstat` 打类型/权限/属主/大小/时间  
2. 不同 umask 下建文件比权限  
3. `futimens` 改 atime/mtime，看 ctime 变  
4. （选）setuid 场景下 `access` 误导  
5. （选）`statx` 取 btime  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | `lstat` 看链接本身；`fstat` 减路径竞争 |
| 2 | atime 读 / mtime 内容 / ctime inode；ctime 不可设 |
| 3 | 最终权限 ≈ mode & ~umask |
| 4 | sticky 目录：防他人删你的文件（`/tmp`） |
| 5 | 别用 `access` 做安全决策 |
| 6 | Linux 要 birth time → `statx` |

---


---

## 参考


- Kerrisk · TLPI Ch15  
- `man 2 stat` · `man 2 chmod` · `man 2 chown` · `man 2 utimensat` · `man 2 umask` · `man 2 access` · `man 2 statx`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

/* Ch15 文件属性 — stat/fstat/lstat 获取 inode 信息。
 * 演示文件类型判断 + 权限/大小/时间戳。
 * 编译: gcc -o ch15_demo ch15_demo.c */

int main(void) {
    struct stat sb;
    if (lstat("/etc/hostname", &sb) < 0) {
        perror("lstat");
        return 1;
    }

    /* 文件类型 */
    if (S_ISREG(sb.st_mode))  printf("type: regular file\n");
    else if (S_ISDIR(sb.st_mode)) printf("type: directory\n");
    else if (S_ISLNK(sb.st_mode)) printf("type: symlink\n");
    else if (S_ISCHR(sb.st_mode)) printf("type: char device\n");
    else if (S_ISBLK(sb.st_mode)) printf("type: block device\n");

    /* 权限 */
    printf("mode: %04o\n", sb.st_mode & 07777);
    printf("size: %ld bytes\n", (long)sb.st_size);
    printf("links: %ld\n", (long)sb.st_nlink);
    printf("uid: %u, gid: %u\n", sb.st_uid, sb.st_gid);

    /* 时间戳 */
    printf("mtime: %s", ctime(&sb.st_mtime));
    printf("atime: %s", ctime(&sb.st_atime));
    printf("ctime: %s", ctime(&sb.st_ctime));

    /* inode 号 */
    printf("inode: %lu\n", (unsigned long)sb.st_ino);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
