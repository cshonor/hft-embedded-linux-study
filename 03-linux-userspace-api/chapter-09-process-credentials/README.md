# TLPI 第 09 章 — Process Credentials

**优先级**：🔴（权限检查 / setuid / 安全铺垫）  
**前置**：[Ch8 Users and Groups](../chapter-08-users-and-groups/README.md) · [Ch24 fork](../chapter-24-process-creation/README.md) · [Ch27 exec](../chapter-27-program-execution/README.md)  
**后置**：[Ch10 Times and Dates](../chapter-10-time/README.md) · [Ch38 特权程序](../chapter-38-secure-privileged/README.md) · [Ch39 Capabilities](../chapter-39-capabilities/README.md)

---

## 小节目录

- [9.1 五类凭证](notes/9.1-real-uid-gid.md)
- [9.2 –9.3 Set-User-ID / Set-Group-ID](notes/9.3-set-user-id.md)
- [9.4 Saved-ID 升降权流程](./notes/9.4-saved-id.md)
- [9.5 获取凭证 API](notes/9.5-fs-uid-gid.md)
- [9.6 –9.7 修改凭证（重难点）](notes/9.6-supplementary-groups.md)
- [9.8 fork / exec 对凭证（必考）](notes/9.8-summary.md)
- [9.9 易混考点](notes/9.1-real-uid-gid.md)

---

## 章节目标


掌握进程持有的全套 UID/GID；理解 **set-user-ID / set-group-ID**；分清特权/非特权下各 `set*id` 规则；知道 Linux 独有的 filesystem ID；为 Ch38 安全特权程序、Ch39 capabilities 打底。

内核侧对应概念：`struct cred`（各 ID 存在进程凭据里）。

---


---

## Ch8 vs Ch9（再强调）


| | Ch8 | Ch9 |
|--|-----|-----|
| 焦点 | `/etc/passwd` 等**账户库** | 进程**当前** R/E/S/F + 补充组 |
| 典型 API | `getpwnam` / `crypt` | `getuid` / `seteuid` / `getresuid` |

---


---

## 练习方向


1. 打印 R/E/S + 补充组（本目录 Demo）  
2. setuid 二进制内 `seteuid` 降权再提权  
3. 对比特权 vs 非特权下 `setuid` vs `seteuid`  
4. `exec` 前后 `getresuid`，观察 Saved-ID  

---


---

## 与后续章节


| 章 | 关联 |
|----|------|
| Ch27 Program Execution | `execve` 再结合 setuid |
| Ch38 Secure Privileged Programs | 凭证误用 → 漏洞 |
| Ch39 Capabilities | 现代细粒度特权，弱化「EUID=0 一把梭」 |

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 权限检查看 **EUID/EGID**（+ 补充组） |
| 2 | 特权 ⇔ **EUID==0** |
| 3 | setuid 位：`exec` 后 EUID=文件属主，RUID 不变，Saved←EUID |
| 4 | Saved-ID：为了能 `seteuid` 来回切换 |
| 5 | `setuid`(特权) 改 R+E+S；非特权多只能动 E |
| 6 | FUID：Linux 遗留，默认=EUID，勿依赖 |

---


---

## 参考


- Kerrisk · TLPI Ch9 Process Credentials  
- `man 7 credentials` · `man 2 setresuid` · `man 2 seteuid` · `man 2 getresuid`


---

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

/* Ch9 进程凭证 — real/effective/saved-set uid/gid。
 * getresuid/getresgid 同时获取三种 uid/gid。
 * 编译: gcc -o ch9_demo ch9_demo.c */

int main(void) {
    uid_t ruid, euid, suid;
    gid_t rgid, egid, sgid;

    if (getresuid(&ruid, &euid, &suid) == 0)
        printf("uid: real=%u, effective=%u, saved=%u\n",
               ruid, euid, suid);

    if (getresgid(&rgid, &egid, &sgid) == 0)
        printf("gid: real=%u, effective=%u, saved=%u\n",
               rgid, egid, sgid);

    /* setuid 特殊语义：
     * - 以 root 运行: 三者都设
     * - 以普通用户运行: 只设 effective（且只能在 real/saved 范围内）
     */
    printf("\nIf running as root, all three uid become the target.\n");
    printf("If running as normal user, only effective uid changes.\n");
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
