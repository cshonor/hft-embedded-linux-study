# TLPI 第 39 章 — Capabilities

**优先级**：🔴（最小特权、容器、替代 SUID）  
**前置**：[Ch9 凭证](../chapter-09-process-credentials/notes.md) · [Ch38 特权安全](../chapter-38-secure-privileged/notes.md) · [Ch16 xattr](../chapter-16-extended-attributes/notes.md)  
**后置**：[Ch40 登录记账](../chapter-40-login-accounting/notes.md)

---

## 小节目录

- [39.1 动机](./notes/39.1-section-39-1.md)
- [39.3 进程能力集（**每线程**一份）](./notes/39.3-process-thread-capabilities.md)
- [39.5 exec 转换（简化）](./notes/39.5-exec.md)
- [39.6 UID 与能力](./notes/39.6-uid.md)
- [39.7 API](./notes/39.7-api.md)

---

## 章节目标


动机；进程 5 集 + 文件能力；exec 转换；Bounding/Ambient；UID 切换影响；libcap / `setcap`；capability-aware vs dumb。

---


---

## 易错清单


1. 能力是**线程**粒度  
2. 从 Permitted 删掉难自愈  
3. Ambient 不服务 root exec 传递  
4. 需 FS xattr 支持文件能力  
5. Bounding 只减  
6. 新项目：文件能力 > SUID-root  

---


---

## 实验清单


1. `setcap`/`getcap` 对比 SUID  
2. libcap 临时 Effective  
3. exec 后 `/proc/.../status`  
4. （选）Bounding / Ambient  

---


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


---

## 参考


- Kerrisk · TLPI Ch39  
- `man 7 capabilities` · `man 3 libcap` · `man 8 setcap`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
