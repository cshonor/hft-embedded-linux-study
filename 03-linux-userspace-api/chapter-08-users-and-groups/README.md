# TLPI 第 08 章 — Users and Groups

**优先级**：🟡→🔴（嵌入式权限 / 安全铺垫）  
**前置**：[Ch6 Processes](../chapter-06-processes/README.md)  
**后置**：[Ch9 进程凭证](../chapter-09-process-credentials/README.md) · [Ch15 文件属性/权限](../chapter-15-file-attributes/README.md)

---

## 小节目录

- [8.1 UID & GID](notes/8.1-passwd-file.md)
- [8.2 `/etc/passwd`（全局可读）](notes/8.1-passwd-file.md)
- [8.3 `/etc/group`（全局可读）](notes/8.3-group-file.md)
- 8.4 `/etc/shadow`（仅 root 可读）
- [8.5 `crypt()` — 密码加密](notes/8.5-password-encryption.md)
- [8.6 附属组 Supplementary Groups](notes/8.6-summary.md)

---

## 章节目标


理解 Linux 用户/组；掌握 `passwd`/`group`/`shadow`、查询 API、`crypt` 身份校验；为 Ch9 进程凭证铺垫。

---


---

## Ch8 vs Ch9 速查


| | Ch8 Users and Groups | Ch9 Process Credentials |
|--|----------------------|-------------------------|
| 焦点 | 账户**数据库**（文件 + 查询） | 进程**运行时** UID/GID 集合 |
| 内容 | passwd/group/shadow、`crypt` | RUID/EUID/SUID、setuid、切换 |
| 不讲 | setuid 程序、权限切换 | 账户文件字段细节 |

---


---

## 易错清单


1. `passwd` 可读；`shadow` 严格限制。  
2. `getpwnam`/`getpwuid`/`crypt` 静态缓冲，连续调用覆盖。  
3. `pw_passwd` 仅占位；真密文在 shadow。  
4. 本章 **无** RUID/EUID/saved UID / setuid 程序。  
5. `getpwnam` 失败时先清 `errno` 再区分「不存在」vs「出错」。

---


---

## 章节链路


```
Ch6  进程是谁在跑
  → Ch8  系统里有哪些用户/组（数据库）
  → Ch9  这个进程此刻用哪套 UID/GID（凭证）
  → Ch15 文件 rwx 如何用 UID/GID 判定
```

---


---

## 双线提示


| 路线 | |
|------|--|
| 嵌入式 | 设备节点/配置文件属主；勿把 shadow 逻辑塞进普通服务 |
| HFT | 少直接碰账户库；跑单用户/专用账号即可；权限切换见 Ch9 |

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | UID 0=root；主组 + 附属组 |
| 2 | passwd 公开；shadow 仅 root；`x` 占位 |
| 3 | get\*nam/uid → 静态缓冲；errno 范式 |
| 4 | crypt(key, salt) 比对 shadow |
| 5 | 账户库 = Ch8；进程凭证 = Ch9 |

---


---

## 参考


- Kerrisk, *The Linux Programming Interface*, **Chapter 8 — Users and Groups**  
- [OUTLINE](../OUTLINE.md) · [Ch9](../chapter-09-process-credentials/README.md) · [Ch15](../chapter-15-file-attributes/README.md)


---

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>

/* Ch8 用户与组 — getuid/getgid/getgroups + getpwuid/getgrgid。
 * 演示获取当前进程的用户/组信息。
 * 编译: gcc -o ch8_demo ch8_demo.c */

int main(void) {
    uid_t uid = getuid();
    gid_t gid = getgid();
    printf("uid=%u, gid=%u\n", uid, gid);

    /* 获取用户名 */
    struct passwd *pw = getpwuid(uid);
    if (pw)
        printf("user: %s (%s)\n", pw->pw_name, pw->pw_gecos);

    /* 获取组名 */
    struct group *gr = getgrgid(gid);
    if (gr)
        printf("group: %s\n", gr->gr_name);

    /* 获取所有附属组 */
    int ngroups = getgroups(0, NULL);
    if (ngroups > 0) {
        gid_t groups[32];
        getgroups(ngroups, groups);
        printf("supplementary groups (%d):", ngroups);
        for (int i = 0; i < ngroups; i++) {
            struct group *g = getgrgid(groups[i]);
            printf(" %s", g ? g->gr_name : "?");
        }
        printf("\n");
    }
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
