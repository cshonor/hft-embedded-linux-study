# TLPI 第 16 章 — Extended Attributes

> 对应目录：`chapter-16-extended-attributes/`  
> 书名原文：**Extended Attributes**  
> ⚠️ **EA = inode 上的键值元数据：** `namespace.name`；ACL / SELinux / file caps 都落在特定命名空间。备份默认常**丢** xattr。

**优先级**：🔴（ACL、capabilities、安全标签的底层载体）  
**前置**：[Ch15 File Attributes](../chapter-15-file-attributes/notes.md)  
**后置**：[Ch17 Access Control Lists](../chapter-17-access-control-lists/notes.md) · [Ch39 Capabilities](../chapter-39-capabilities/notes.md) · [Ch38 特权与安全](../chapter-38-secure-privileged/notes.md)

---

## 章节目标

理解 xattr 键值模型与四大命名空间；掌握 set/get/remove/list 系统调用；熟悉权限与大小限制；为 ACL、文件能力、安全标签打底。

---

## 16.1 概念

标准 inode 字段固定；**扩展属性（EA）** 附加任意 `key → value`（多为小元数据，非大文件内容替代）。

- 对象：普通文件、目录、链接、设备、FIFO 等（视 FS）  
- 支持：ext4 / XFS / Btrfs / tmpfs 等；**不保证所有 FS** → 处理 `ENOTSUP`  
- 符号链接：默认跟随目标；操作链接本体用 `l*` 系列（FS 对 symlink EA 支持常很弱）

### 键名：`namespace.name`

| 命名空间 | 谁用 | 权限要点 |
|----------|------|----------|
| **`user.*`** | 普通自定义元数据 | 受文件 r/w 权限约束 |
| **`trusted.*`** | 管理员私有 | 仅 root（EUID） |
| **`security.*`** | SELinux、`security.capability` 等 | 多为内核/特权 |
| **`system.*`** | 内核用途；**ACL** 在此（`system.posix_acl_*`） | 普通用户一般不能乱改 |

> POSIX ACL 实质是存在 `system.*` 里的 EA → [Ch17](../chapter-17-access-control-lists/notes.md)。

---

## 16.2 系统调用组

```c
#include <sys/xattr.h>

int setxattr(const char *path, const char *name,
             const void *value, size_t size, int flags);
int lsetxattr(...);   /* 不跟随链接 */
int fsetxattr(int fd, ...);

ssize_t getxattr(const char *path, const char *name, void *value, size_t size);
ssize_t lgetxattr(...);
ssize_t fgetxattr(int fd, ...);

int removexattr(const char *path, const char *name);
int lremovexattr(...);
int fremovexattr(int fd, ...);

ssize_t listxattr(const char *path, char *list, size_t size);
ssize_t llistxattr(...);
ssize_t flistxattr(int fd, ...);
```

### `setxattr` flags

| flag | 行为 |
|------|------|
| `0`（默认） | 无则建，有则换 |
| `XATTR_CREATE` | 仅新建；已存在 → `EEXIST` |
| `XATTR_REPLACE` | 仅更新；不存在 → `ENODATA`（文档/历史上也称 ENOATTR） |

---

## 16.3 `listxattr` 格式

缓冲区为连续 `\0` 结尾的键名：

```text
user.foo\0user.bar\0
```

返回值 = 总字节数。范式：

1. `listxattr(path, NULL, 0)`（或 `size=0`）得长度  
2. 分配缓冲再读；`getxattr(..., size=0)` 同理得 value 长度  

`listxattr` **只有键**，value 需再 `getxattr`。

Demo：[`code/xattr_demo.c`](./code/xattr_demo.c)

---

## 16.4 权限（Linux 要点）

| 命名空间 | set / remove | get |
|----------|--------------|-----|
| `user.*` | 文件**写**权限 | 文件**读**权限 |
| `trusted.*` | 仅 root | 仅 root |
| `security.*` / `system.*` | 多为特权/内核 | 视策略 |

`user.*`：**不是**「创建者永久拥有」——换人改权限后规则仍按当前 r/w。

---

## 16.5 限制与踩坑

1. 单文件 EA **总量**有限（ext4 inode 内空间有限，过大则外挂块）  
2. 单 value 有上限 → 只适小元数据  
3. 旧挂载可能要 `user_xattr`；ext4 默认通常已开  
4. **`cp` 默认不带 EA**；`cp --preserve=xattr`、`rsync -X`  
5. 硬链接共享 inode → xattr 一改全见  
6. 程序须处理 `ENOTSUP` / `EOPNOTSUPP`  

---

## 16.6 命令行

```bash
setfattr -n user.comment -v "test tag" file.txt
getfattr -n user.comment file.txt
getfattr -d file.txt          # dump
```

---

## 16.7 典型用途

- 自定义标签 / 上传者等元信息（`user.*`）  
- SELinux 上下文、`security.capability`  
- POSIX ACL（`system.posix_acl_*`）  
- 备份工具附加元数据  

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

## 16.9 易错清单

1. 键不存在：`ENODATA`（ENOATTR）  
2. `getxattr`/`listxattr` 先 `size=0` 探长度  
3. list 无 value，要逐个 get  
4. 备份丢 EA → ACL/caps 一起丢  
5. 勿假设全 FS 支持 xattr  
6. 默认 `setxattr` 跟随符号链接  

---

## 练习

1. 对 `user.*` 做增删改查 + `listxattr`  
2. 硬链接共享；`cp` 默认 vs `--preserve=xattr`  
3. 非 root 写 `trusted.*` → 拒绝  
4. （选）看目录上是否已有 `system.posix_acl_*`（有 ACL 时）  

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

## 参考

- Kerrisk · TLPI Ch16  
- `man 2 setxattr` · `man 2 getxattr` · `man 2 listxattr` · `man 7 xattr` · `man 1 getfattr`
