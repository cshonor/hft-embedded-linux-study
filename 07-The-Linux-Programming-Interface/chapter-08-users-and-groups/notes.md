# TLPI 第 08 章 — Users and Groups

> 对应目录：`chapter-08-users-and-groups/`  
> 书名原文：**Users and Groups**  
> ⚠️ **本章只讲系统账户数据库**；进程 RUID/EUID、setuid 程序全在 [Ch9 Process Credentials](../chapter-09-process-credentials/notes.md)。

**优先级**：🟡→🔴（嵌入式权限 / 安全铺垫）  
**前置**：[Ch6 Processes](../chapter-06-processes/notes.md)  
**后置**：[Ch9 进程凭证](../chapter-09-process-credentials/notes.md) · [Ch15 文件属性/权限](../chapter-15-file-attributes/notes.md)

---

## 章节目标

理解 Linux 用户/组；掌握 `passwd`/`group`/`shadow`、查询 API、`crypt` 身份校验；为 Ch9 进程凭证铺垫。

---

## Ch8 vs Ch9 速查

| | Ch8 Users and Groups | Ch9 Process Credentials |
|--|----------------------|-------------------------|
| 焦点 | 账户**数据库**（文件 + 查询） | 进程**运行时** UID/GID 集合 |
| 内容 | passwd/group/shadow、`crypt` | RUID/EUID/SUID、setuid、切换 |
| 不讲 | setuid 程序、权限切换 | 账户文件字段细节 |

---

## 8.1 UID & GID

| | |
|--|--|
| **UID** | 用户数值 ID；`0` = root |
| 系统账号 | 常见约 1–999（发行版略有差异）；守护进程，常无登录 shell |
| 普通用户 | 常见 ≥1000 |
| **GID** | 组 ID；每用户有**主组**；可加多个**附属组** |
| 用途 | 文件所有权、资源访问判定 |

---

## 8.2 `/etc/passwd`（全局可读）

格式（7 字段，冒号分隔）：

```
name:x:uid:gid:comment:home:shell
```

| 字段 | 含义 |
|------|------|
| name | 登录名 |
| x | 密码占位；真密文在 `/etc/shadow` |
| uid / gid | 用户 ID / 主组 ID |
| comment | GECOS / finger |
| home | 家目录 |
| shell | 登录 shell；`/sbin/nologin` 等禁止登录 |

```c
#include <pwd.h>
struct passwd {
    char *pw_name;
    char *pw_passwd;   /* 占位，非真密文 */
    uid_t pw_uid;
    gid_t pw_gid;
    char *pw_gecos;
    char *pw_dir;
    char *pw_shell;
};
```

### 查询 API

```c
struct passwd *getpwnam(const char *name);
struct passwd *getpwuid(uid_t uid);

void setpwent(void);
struct passwd *getpwent(void);
void endpwent(void);
```

找不到用户：返回 `NULL`，**可能不改 errno**。范式：

```c
errno = 0;
pwd = getpwnam("nobody");
if (pwd == NULL) {
    if (errno == 0) { /* 不存在 */ }
    else { /* 系统错误 */ }
}
```

> 返回值指向**静态缓冲**；连续调用会覆盖；多线程不安全。

---

## 8.3 `/etc/group`（全局可读）

```
groupname:x:gid:member1,member2
```

```c
#include <grp.h>
struct group {
    char *gr_name;
    char *gr_passwd;
    gid_t gr_gid;
    char **gr_mem;   /* 附属组成员名列表 */
};
```

```c
struct group *getgrnam(const char *name);
struct group *getgrgid(gid_t gid);
void setgrent(void);
struct group *getgrent(void);
void endgrent(void);
```

Demo：[`code/users_groups.c`](./code/users_groups.c)

---

## 8.4 `/etc/shadow`（仅 root 可读）

把加密密码从公开 `passwd` 拆出。

字段概要：`username:encrypted:…`（含最后修改、最小/最大间隔、警告、宽限、失效等）。

```c
#include <shadow.h>
struct spwd {
    char *sp_namp;
    char *sp_pwdp;   /* 加密密码 */
    long sp_lstchg, sp_min, sp_max, sp_warn, sp_inact, sp_expire;
};
```

```c
struct spwd *getspnam(const char *name);
void setspent(void);
struct spwd *getspent(void);
void endspent(void);
```

普通用户调 `getspnam` → 常失败，`errno=EACCES`。

---

## 8.5 `crypt()` — 密码加密

```c
#define _XOPEN_SOURCE
#include <unistd.h>
char *crypt(const char *key, const char *salt);
```

| | |
|--|--|
| key | 明文密码 |
| salt | 取自 shadow 密文头部（算法标识 + 盐） |
| 返回 | 静态缓冲；**勿 free**；非线程安全 |

认证：`getspnam` → 取 `sp_pwdp` 作 salt → `crypt` → 与密文比字符串。  
验证后立刻**覆盖清空**明文缓冲。

Demo：[`code/check_password.c`](./code/check_password.c)（需能读 shadow，通常 root）

---

## 8.6 附属组 Supplementary Groups

进程可有一组附属 GID，访问多组资源时一并参与校验（与主组一起）。

```c
#include <grp.h>
int getgroups(int size, gid_t list[]);
int setgroups(size_t size, const gid_t *list);  /* 特权 */
```

> 附属组 ≠ 主组；文件权限看有效 GID **+** 全部附属组。细节与「当前进程凭证」仍属 Ch9。

---

## 易错清单

1. `passwd` 可读；`shadow` 严格限制。  
2. `getpwnam`/`getpwuid`/`crypt` 静态缓冲，连续调用覆盖。  
3. `pw_passwd` 仅占位；真密文在 shadow。  
4. 本章 **无** RUID/EUID/saved UID / setuid 程序。  
5. `getpwnam` 失败时先清 `errno` 再区分「不存在」vs「出错」。

---

## 章节链路

```
Ch6  进程是谁在跑
  → Ch8  系统里有哪些用户/组（数据库）
  → Ch9  这个进程此刻用哪套 UID/GID（凭证）
  → Ch15 文件 rwx 如何用 UID/GID 判定
```

---

## 双线提示

| 路线 | |
|------|--|
| 嵌入式 | 设备节点/配置文件属主；勿把 shadow 逻辑塞进普通服务 |
| HFT | 少直接碰账户库；跑单用户/专用账号即可；权限切换见 Ch9 |

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

## 参考

- Kerrisk, *The Linux Programming Interface*, **Chapter 8 — Users and Groups**  
- [OUTLINE](../OUTLINE.md) · [Ch9](../chapter-09-process-credentials/notes.md) · [Ch15](../chapter-15-file-attributes/notes.md)
