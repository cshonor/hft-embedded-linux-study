# TLPI 第 17 章 — Access Control Lists

> 对应目录：`chapter-17-access-control-lists/`  
> 书名原文：**Access Control Lists**  
> ⚠️ **细粒度权限：** 传统 rwx 只有 owner/group/other；POSIX ACL 可为任意用户/组授权。底层存在 `system.posix_acl_*` xattr（见 [Ch16](../chapter-16-extended-attributes/notes.md)）。

**优先级**：🔴（多用户共享目录、备份丢 ACL、chmod↔MASK 陷阱）  
**前置**：[Ch16 Extended Attributes](../chapter-16-extended-attributes/notes.md)  
**后置**：[Ch18 Directories and Links](../chapter-18-directories-links/notes.md) · [Ch38](../chapter-38-secure-privileged/notes.md) · [Ch39 Capabilities](../chapter-39-capabilities/notes.md)

> 标准背景：POSIX.1e 草案（正式标准废弃，Linux 沿用实现）。

---

## 章节目标

掌握 ACE 标签、最小/扩展 ACL、`ACL_MASK`、Access vs Default ACL；会用 libacl（`-lacl`）；理清内核判定顺序与 `chmod`/`umask`/`ls -l` 交互陷阱。

---

## 17.1 基础概念

ACL = 多条 **ACE**：`tag + [qualifier uid/gid] + 权限(rwx)`。

### 六种 Tag

| 常量 | 文本 | 说明 |
|------|------|------|
| `ACL_USER_OBJ` | `user::` | 所有者（固定 1） |
| `ACL_USER` | `user:uid:` | 命名用户（可多条） |
| `ACL_GROUP_OBJ` | `group::` | 属组（固定 1） |
| `ACL_GROUP` | `group:gid:` | 命名组（可多条） |
| `ACL_MASK` | `mask::` | **有效权限上限**（扩展 ACL 必需） |
| `ACL_OTHER` | `other::` | 其他人（固定 1） |

### 两类 ACL

| 类型 | 作用 | xattr |
|------|------|-------|
| **Access ACL** | 控制该对象访问 | `system.posix_acl_access` |
| **Default ACL** | **仅目录**；不控制本目录访问，供**新建**子对象继承 | `system.posix_acl_default` |

### 最小 vs 扩展

| | 组成 | MASK |
|--|------|------|
| **最小 ACL** | USER_OBJ + GROUP_OBJ + OTHER | 不需要（≈ 传统 mode） |
| **扩展 ACL** | 另有 `ACL_USER` / `ACL_GROUP` | **必须有** `ACL_MASK` |

---

## 17.2 `ACL_MASK`（核心坑）

MASK 限制：**命名用户、命名组、GROUP_OBJ** 的有效权限上限。

```text
有效权限 = ACE 权限 & MASK
```

例：`user:alice:rwx` + `mask::r--` → alice 实际只有 `r--`。

**`ls -l` 现象：** 存在扩展 ACL 时，group 列显示的是 **MASK**，不是 GROUP_OBJ；行末常有 `+`。

---

## 17.3 内核判定顺序（示意）

1. root → 放行（执行还需有 x，细节见手册）  
2. EUID == 所有者 → `USER_OBJ`  
3. 匹配某 `ACL_USER` → 权限 & MASK  
4. EGID/附加组匹配 `ACL_GROUP` / `GROUP_OBJ` → & MASK  
5. → `ACL_OTHER`  
6. 否则拒绝  

---

## 17.4 Default ACL 继承

父目录设 Default ACL 后：

| 新建对象 | 行为 |
|----------|------|
| 普通文件 | Default → 自身 Access ACL；创建 `mode` 仍约束；**umask 通常不再按传统方式生效** |
| 子目录 | Access + **自己的 Default** 都从父 Default 继承（可继续向下传） |

> Default **不改**目录里**已有**文件，只影响之后新建的对象。

---

## 17.5 libacl API（链接 `-lacl`）

```c
#include <sys/acl.h>
#include <acl/libacl.h>   /* 视发行版；部分声明在 sys/acl.h */

acl_t acl_get_file(const char *path, acl_type_t type);
int   acl_set_file(const char *path, acl_type_t type, acl_t acl);
/* type: ACL_TYPE_ACCESS / ACL_TYPE_DEFAULT */

int acl_get_entry(acl_t acl, int entry_id, acl_entry_t *entry_p);
int acl_create_entry(acl_t *acl_p, acl_entry_t *entry_p);
int acl_set_tag_type(acl_entry_t entry_d, acl_tag_t tag_type);
int acl_set_qualifier(acl_entry_t entry_d, const void *qualifier_p);
int acl_set_permset(acl_entry_t entry_d, acl_permset_t permset_d);

int acl_valid(acl_t acl);
void acl_free(void *obj_p);
```

编程要点：

1. 扩展 ACL **手写 MASK**  
2. 写入前 `acl_valid()`  
3. 用完 `acl_free()`  
4. FS 需支持 ACL（现代 ext4 等通常默认可用）  

Demo：[`code/print_acl.c`](./code/print_acl.c) · [`code/set_named_user_acl.c`](./code/set_named_user_acl.c)

---

## 17.6 命令行

```bash
getfacl file
getfacl -d dir

setfacl -m u:alice:rw file
setfacl -m g:dev:r-x dir
setfacl -d -m u:bob:rwx /project   # default ACL

setfacl -x u:alice file
setfacl -b file                    # 清 access ACL
setfacl -k dir                     # 仅清 default ACL
```

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

## 17.8 易错清单

1. 备份丢 ACL → 权限「突然不对」  
2. `chmod` + 扩展 ACL → 动的是 MASK；`ls` group 列是 MASK  
3. 有 Default ACL 时 umask 行为改变  
4. 硬链接共享 ACL；软链接无自身 ACL（跟目标）  
5. NFS ACL 兼容性慎用  
6. libacl **不可移植**到典型 BSD/macOS 同一套 API  

---

## 练习

1. 遍历打印 Access ACE（简易 getfacl）  
2. 写扩展 ACL：命名用户 + MASK，再 `getfacl`/`ls -l+`  
3. 目录 Default ACL → 新建文件是否继承  
4. 带 ACL 文件 `chmod`，观察 group 列变 MASK  

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

## 参考

- Kerrisk · TLPI Ch17  
- `man 3 acl_get_file` · `man 5 acl` · `man 1 getfacl` · `man 1 setfacl`
