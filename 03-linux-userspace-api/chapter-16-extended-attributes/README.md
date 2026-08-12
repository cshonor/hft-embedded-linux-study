# TLPI 第 16 章 — Extended Attributes

**优先级**：🔴（ACL、capabilities、安全标签的底层载体）  
**前置**：[Ch15 File Attributes](../chapter-15-file-attributes/notes.md)  
**后置**：[Ch17 Access Control Lists](../chapter-17-access-control-lists/notes.md) · [Ch39 Capabilities](../chapter-39-capabilities/notes.md) · [Ch38 特权与安全](../chapter-38-secure-privileged/notes.md)

---

## 小节目录

- [16.1 概念](./notes/16.1-concepts.md)
- [16.2 系统调用组](./notes/16.2-syscall-group.md)
- [16.3 `listxattr` 格式](./notes/16.3-listxattr.md)
- [16.4 权限（Linux 要点）](./notes/16.4-permission.md)
- [16.5 限制与踩坑](./notes/16.5-limits.md)
- [16.6 命令行](./notes/16.6-section-16-6.md)
- [16.7 典型用途](./notes/16.7-section-16-7.md)

---

## 章节目标


理解 xattr 键值模型与四大命名空间；掌握 set/get/remove/list 系统调用；熟悉权限与大小限制；为 ACL、文件能力、安全标签打底。

---


---

## 16.8 速查：命名空间 · 调用族


| | path | 不跟随链接 | fd |
|--|------|------------|-----|
| set | `setxattr` | `lsetxattr` | `fsetxattr` |
| get | `getxattr` | `lgetxattr` | `fgetxattr` |
| remove | `removexattr` | `lremovexattr` | `fremovexattr` |
| list | `listxattr` | `llistxattr` | `flistxattr` |

| NS | 一句话 |
|----|--------|
| `user` | 用户元数据；受文件 r/w |
| `trusted` | 仅 root |
| `security` | 安全模块 / caps |
| `system` | 内核；含 ACL |

---


---

## 16.9 易错清单


1. 键不存在：`ENODATA`（ENOATTR）  
2. `getxattr`/`listxattr` 先 `size=0` 探长度  
3. list 无 value，要逐个 get  
4. 备份丢 EA → ACL/caps 一起丢  
5. 勿假设全 FS 支持 xattr  
6. 默认 `setxattr` 跟随符号链接  

---


---

## 练习


1. 对 `user.*` 做增删改查 + `listxattr`  
2. 硬链接共享；`cp` 默认 vs `--preserve=xattr`  
3. 非 root 写 `trusted.*` → 拒绝  
4. （选）看目录上是否已有 `system.posix_acl_*`（有 ACL 时）  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 键格式 `namespace.name`；日常用 `user.*` |
| 2 | ACL / caps 分别在 `system.*` / `security.*` |
| 3 | list 是 `\0` 分隔键表；先 size=0 再分配 |
| 4 | `cp`/`rsync` 默认可能丢 EA |
| 5 | 硬链接共享；处理 `ENOTSUP` |
| 6 | trusted 仅 root；user 看文件 r/w |

---


---

## 参考


- Kerrisk · TLPI Ch16  
- `man 2 setxattr` · `man 2 getxattr` · `man 2 listxattr` · `man 7 xattr` · `man 1 getfattr`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/xattr.h>
#include <string.h>

/* Ch16 扩展属性 — getxattr/setxattr/listxattr。
 * xattr 是文件 inode 上附带的 key-value 元数据。
 * 编译: gcc -o ch16_demo ch16_demo.c
 * 需要 attr 包: apt install attr */

int main(void) {
    const char *path = "/tmp/ch16_test.txt";
    FILE *fp = fopen(path, "w");
    if (!fp) { perror("fopen"); return 1; }
    fprintf(fp, "test\n");
    fclose(fp);

    /* 设置扩展属性 */
    if (setxattr(path, "user.comment", "hello xattr", 11, 0) < 0)
        perror("setxattr (need user namespace + ext4/xfs)");

    /* 列出所有扩展属性 */
    char list[256];
    ssize_t llen = listxattr(path, list, sizeof(list));
    if (llen > 0) {
        printf("xattr keys:\n");
        for (ssize_t i = 0; i < llen; ) {
            printf("  %s\n", list + i);
            i += strlen(list + i) + 1;
        }
    }

    /* 读取扩展属性值 */
    char val[256];
    ssize_t vlen = getxattr(path, "user.comment", val, sizeof(val) - 1);
    if (vlen >= 0) {
        val[vlen] = '\0';
        printf("user.comment = %s\n", val);
    }

    remove(path);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
