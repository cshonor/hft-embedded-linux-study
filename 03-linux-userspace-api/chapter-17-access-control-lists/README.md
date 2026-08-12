# TLPI 第 17 章 — Access Control Lists

**优先级**：🔴（多用户共享目录、备份丢 ACL、chmod↔MASK 陷阱）  
**前置**：[Ch16 Extended Attributes](../chapter-16-extended-attributes/notes.md)  
**后置**：[Ch18 Directories and Links](../chapter-18-directories-links/notes.md) · [Ch38](../chapter-38-secure-privileged/notes.md) · [Ch39 Capabilities](../chapter-39-capabilities/notes.md)

---

## 小节目录

- [17.1 基础概念](./notes/17.1-concepts.md)
- [17.2 `ACL_MASK`（核心坑）](./notes/17.2-aclmask.md)
- [17.3 内核判定顺序（示意）](./notes/17.3-section-17-3.md)
- [17.4 Default ACL 继承](./notes/17.4-default-acl.md)
- [17.5 libacl API（链接 `-lacl`）](./notes/17.5-libacl-api-lacl.md)
- [17.6 命令行](./notes/17.6-section-17-6.md)

---

## 章节目标


掌握 ACE 标签、最小/扩展 ACL、`ACL_MASK`、Access vs Default ACL；会用 libacl（`-lacl`）；理清内核判定顺序与 `chmod`/`umask`/`ls -l` 交互陷阱。

---


---

## 17.7 速查：Access vs Default · chmod 影响


| | Access ACL | Default ACL |
|--|------------|-------------|
| 对象 | 文件/目录 | **仅目录** |
| 控制本对象访问？ | 是 | **否** |
| 继承 | — | 新建文件/子目录 |

| 对扩展 ACL 做 `chmod` | 效果 |
|----------------------|------|
| 改「属组」那三位（ls 上的 group） | 实际改的是 **MASK**，不是 GROUP_OBJ |
| 最小 ACL | 与传统一致，改 GROUP_OBJ |

| 备份 | |
|------|--|
| 默认 `cp` | 易丢 ACL（同 xattr） |
| 保留 | `cp --preserve=xattr` / `rsync -A`（及 xattr 相关选项） |

---


---

## 17.8 易错清单


1. 备份丢 ACL → 权限「突然不对」  
2. `chmod` + 扩展 ACL → 动的是 MASK；`ls` group 列是 MASK  
3. 有 Default ACL 时 umask 行为改变  
4. 硬链接共享 ACL；软链接无自身 ACL（跟目标）  
5. NFS ACL 兼容性慎用  
6. libacl **不可移植**到典型 BSD/macOS 同一套 API  

---


---

## 练习


1. 遍历打印 Access ACE（简易 getfacl）  
2. 写扩展 ACL：命名用户 + MASK，再 `getfacl`/`ls -l+`  
3. 目录 Default ACL → 新建文件是否继承  
4. 带 ACL 文件 `chmod`，观察 group 列变 MASK  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | ACL = ACE 列表；扩展 ACL 必须有 MASK |
| 2 | 有效权限 = ACE & MASK |
| 3 | `ls -l` 有 `+` 时 group 列常是 MASK |
| 4 | Default ACL 只影响新建；目录专属 |
| 5 | 存于 `system.posix_acl_*` xattr |
| 6 | `chmod` 扩展 ACL 时改 MASK，勿当 GROUP_OBJ |

---


---

## 参考


- Kerrisk · TLPI Ch17  
- `man 3 acl_get_file` · `man 5 acl` · `man 1 getfacl` · `man 1 setfacl`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/acl.h>
#include <errno.h>

/* Ch17 访问控制列表 (ACL) — acl_get_file/acl_set_file。
 * ACL 比传统 rwx 更细粒度：可给特定用户/组单独授权。
 * 编译: gcc -o ch17_demo ch17_demo.c -lacl
 * 需要安装: apt install libacl1-dev */

int main(void) {
    const char *path = "/tmp/ch17_test.txt";
    FILE *fp = fopen(path, "w");
    if (!fp) { perror("fopen"); return 1; }
    fprintf(fp, "test\n");
    fclose(fp);

    /* 获取文件 ACL */
    acl_t acl = acl_get_file(path, ACL_TYPE_ACCESS);
    if (acl == NULL) {
        perror("acl_get_file (need libacl)");
        remove(path);
        return 1;
    }

    /* 打印 ACL 文本形式 */
    char *text = acl_to_text(acl, NULL);
    if (text) {
        printf("Current ACL:\n%s\n", text);
        acl_free(text);
    }
    acl_free(acl);

    /* 创建新 ACL 条目: 给 uid 1000 读写权限 */
    acl = acl_from_text("u::rw,g::r,o::r,u:1000:rw,m::rw");
    if (acl) {
        if (acl_set_file(path, ACL_TYPE_ACCESS, acl) == 0)
            printf("ACL updated: uid 1000 gets rw\n");
        else
            perror("acl_set_file");
        acl_free(acl);
    }

    remove(path);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
