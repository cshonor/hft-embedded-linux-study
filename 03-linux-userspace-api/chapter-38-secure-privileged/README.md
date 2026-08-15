# TLPI 第 38 章 — Writing Secure Privileged Programs

**优先级**：🔴（SUID、daemon 降权、攻击面）  
**前置**：[Ch9 凭证](../chapter-09-process-credentials/README.md) · [Ch37 Daemons](../chapter-37-daemons/README.md)  
**后置**：[Ch39 Capabilities](../chapter-39-capabilities/README.md)

---

## 小节目录

- [38.1 两类特权程序](notes/38.1-is-a-set-user-id-or-set-group-id-program.md)
- [38.2 丢弃与恢复（核心）](notes/38.2-operate-with-least-privilege.md)
- [38.3 安全准则（精要）](notes/38.3-be-careful-when-executing-a-program.md)

---

## 章节目标


SUID/SGID 安全模型；`setuid` vs `seteuid`；临时/永久丢权；TOCTOU、环境、符号链接、shell 注入；多层准则；衔 Capability。

---


---

## 易错清单


1. root 下用 `setuid`「临时」降权 → 回不去  
2. `system` + 脏 PATH  
3. `stat`→`open` TOCTOU  
4. 全程持 root 跑复杂逻辑  
5. fork 继承 UID；SUID 行为在 exec 时生效  

---


---

## 实验清单


1–2. Ch9 临时/永久降权  
3. `open`+`fstat` vs TOCTOU  
4. （选）PATH 劫持对比  
5. `O_NOFOLLOW`  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 权限看 EUID；Saved 管恢复 |
| 2 | seteuid 临时；setuid(root) 永久 |
| 3 | 少 SUID → Capability / daemon |
| 4 | 禁 system；绝对路径 exec |
| 5 | open+fstat，防 TOCTOU |
| 6 | 清环境、校验输入、关多余 fd |

---


---

## 参考


- Kerrisk · TLPI Ch38  
- [Ch9 notes](../chapter-09-process-credentials/README.md) · `man 7 credentials` · `man 2 seteuid`


---

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

/* Ch38 安全与特权编程 — 降权 + 最小权限原则。
 * 演示 setuid 程序如何临时/永久放弃特权。
 * 编译: gcc -o ch38_demo ch38_demo.c */

int main(void) {
    uid_t ruid = getuid();    /* 实际 uid */
    uid_t euid = geteuid();   /* 有效 uid */

    printf("Before: real=%u, effective=%u\n", ruid, euid);

    if (euid == 0) {
        printf("Running as root (or setuid-root)\n");

        /* 永久降权: 先设 effective，再设 saved-set */
        /* 假设要降为 uid 1000 */
        uid_t target = 1000;
        gid_t target_gid = 1000;

        /* 步骤1: 临时降 effective uid */
        seteuid(target);
        printf("After seteuid(%u): euid=%u\n", target, (unsigned)geteuid());

        /* 步骤2: 需要特权时临时恢复 */
        seteuid(0);
        printf("Restored: euid=%u\n", (unsigned)geteuid());

        /* 步骤3: 永久放弃 root (三连设) */
        setgid(target_gid);
        setuid(target);
        printf("After permanent drop: real=%u, eff=%u\n",
               (unsigned)getuid(), (unsigned)geteuid());
        printf("Cannot regain root now\n");
    } else {
        printf("Running as normal user (uid=%u)\n", euid);
        printf("Secure programming: validate all input, minimize privileges\n");
    }
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
