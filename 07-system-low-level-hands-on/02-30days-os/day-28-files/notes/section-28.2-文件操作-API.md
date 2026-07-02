## ② 文件操作 API · EDX 21–25

此前 **仅内核** `file_loadfile` / `dir` — app **不能自主读盘**。

| EDX | 功能 |
|-----|------|
| **21** | **open** |
| **22** | **close** |
| **23** | **seek** |
| **24** | **get size** |
| **25** | **read** |

**封装 FAT12**（Day 18–19）— app 经 **INT 0x40** 访问，**不直接摸磁盘**。

**`typeipl.hrb`：** 外部 app **独立读文件并显示** — 文件 I/O **用户态化**。

→ [Day 18/19 FAT](../day-18-dir/) · [06-The-Linux-Programming-Interface open/read/lseek](../../../../06-The-Linux-Programming-Interface/)

---
