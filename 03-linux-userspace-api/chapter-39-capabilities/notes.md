# TLPI 第 39 章 — Capabilities

> 对应目录：`chapter-39-capabilities/`  
> （勿用 `chapter-39-linux-capabilities` — 与 [CHAPTER-MAP](../CHAPTER-MAP.md) 一致）  
> 书名原文：**Capabilities**  
> ⚠️ **把 root 拆成可单独授予的原子特权。** 权限检查看 **Effective**；Permitted 是上限。优先 **文件能力** 替代 SUID-root。非 POSIX（Linux 实现）。

**优先级**：🔴（最小特权、容器、替代 SUID）  
**前置**：[Ch9 凭证](../chapter-09-process-credentials/notes.md) · [Ch38 特权安全](../chapter-38-secure-privileged/notes.md) · [Ch16 xattr](../chapter-16-extended-attributes/notes.md)  
**后置**：[Ch40 登录记账](../chapter-40-login-accounting/notes.md)

---

## 章节目标

动机；进程 5 集 + 文件能力；exec 转换；Bounding/Ambient；UID 切换影响；libcap / `setcap`；capability-aware vs dumb。

---

## 39.1 动机

传统：UID=0 全有 / 非 0 全无 → 绑 80 端口也得整包 root。  
Capability：如 `CAP_NET_BIND_SERVICE`、`CAP_NET_RAW`、`CAP_SYS_TIME`、`CAP_DAC_OVERRIDE`…

---

## 39.3 进程能力集（**每线程**一份）

| 集合 | 作用 |
|------|------|
| **Permitted** | 可拥有的上限；可从中抬到 Effective；**删掉难自恢复**（除非再 exec 文件能力） |
| **Effective** | **内核检查用**；按需开关 |
| **Inheritable** | fork 继承；参与 exec 算新 Permitted |
| **Bounding** | exec 能拿到的全局上界；**只删不增**；容器常用 |
| **Ambient**（4.3+） | 非 root 跨 exec 传能力；root exec 时清空 |

### 文件能力（xattr）

| 项 | |
|----|--|
| File Permitted | 并入新进程 Permitted（∩ Bounding） |
| File Inheritable | 与进程 Inheritable 与后并入 |
| File Effective | **1 bit**：1 → 新 Effective=新 Permitted；0 → Effective 空（或 Ambient） |

`setcap 'cap_net_raw+ep' ./ping` · `getcap` · `/proc/$$/status` 的 `Cap*`。

---

## 39.5 exec 转换（简化）

```text
新 P ≈ (旧 Inh ∩ File Inh) ∪ (File Prm ∩ Bounding) ∪ Ambient
新 E ≈ FileEff ? 新P : Ambient
新 Inh ≈ 旧 Inh
```

Ambient 须同时在进程 Prm 与 Inh 中；EUID=0 的 exec 清空 Ambient。

---

## 39.6 UID 与能力

| 切换 | 常见效果 |
|------|----------|
| → EUID=0 | 兼容语义：常获满 Permitted |
| 0 → 非 0 | 常清 Effective；Permitted 可保留 → 再抬 Effective |

范式：保留 Prm、清 Eff → 业务 → 临时抬 Eff → 再清。

---

## 39.7 API

底层：`capget`/`capset`。推荐 **libcap**（`-lcap`）：

```c
cap_t cap_get_proc(void);
int cap_set_proc(cap_t);
int cap_set_flag(..., CAP_EFFECTIVE, ..., CAP_SET|CAP_CLEAR);
void cap_free(cap_t);
```

| 程序类型 | |
|----------|--|
| capability-aware | 自己抬/清 Effective |
| capability-dumb | 靠文件 Effective bit=1 |

Demo：[`code/cap_view.c`](./code/cap_view.c)

---

## 易错清单

1. 能力是**线程**粒度  
2. 从 Permitted 删掉难自愈  
3. Ambient 不服务 root exec 传递  
4. 需 FS xattr 支持文件能力  
5. Bounding 只减  
6. 新项目：文件能力 > SUID-root  

---

## 实验清单

1. `setcap`/`getcap` 对比 SUID  
2. libcap 临时 Effective  
3. exec 后 `/proc/.../status`  
4. （选）Bounding / Ambient  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | Effective 才真正授权 |
| 2 | Permitted = 上限；Bounding = exec 天花板 |
| 3 | 文件能力存在 xattr |
| 4 | File Effective 是 1 bit |
| 5 | 替 SUID-root 用文件能力 |
| 6 | 按需抬 Eff，用完清掉 |

---

## 参考

- Kerrisk · TLPI Ch39  
- `man 7 capabilities` · `man 3 libcap` · `man 8 setcap`
