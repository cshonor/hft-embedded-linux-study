# 第 16 章：非阻塞式 I/O（厚版）

> [Ch 14](../Chapter14_AdvancedIO_Func/study.md) · **Ch 16** · 阶段二收束  
> 逐节：`16.x_*.md`

> **说明**：上传资料截至第 8 章；第 16 章按 UNP 第 3 版体系整理，请与全本对照验证。

## 本章目标

掌握 **O_NONBLOCK** 四类操作语义、**str_cli 全非阻塞**缓冲状态机、**非阻塞 connect**（EINPROGRESS、SO_ERROR）、Web 并发客户、**非阻塞 accept** 防死锁。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 16.1 | [16.1_Overview](./16.1_Overview.md) | 四类阻塞操作 |
| 16.2 | [16.2_NonBlock_Read_Write](./16.2_NonBlock_Read_Write.md) | to/fr 缓冲 + select |
| 16.3 | [16.3_NonBlock_Connect](./16.3_NonBlock_Connect.md) | EINPROGRESS 与三大优势 |
| 16.4 | [16.4_NonBlock_Connect_TimeClient](./16.4_NonBlock_Connect_TimeClient.md) | connect_nonb + SO_ERROR |
| 16.5 | [16.5_NonBlock_Connect_WebClient](./16.5_NonBlock_Connect_WebClient.md) | 并行 HTTP 客户 |
| 16.6 | [16.6_NonBlock_Accept](./16.6_NonBlock_Accept.md) | RST 竞态、强制非阻塞 |
| 16.7 | [16.7_Summary](./16.7_Summary.md) | 全章收束 |

---

## 一章速记

```text
fcntl O_NONBLOCK：读/accept 无数据→EWOULDBLOCK；connect→EINPROGRESS
str_cli：stdin/stdout/sock 全非阻塞；to/fr + iptr/optr 驱动 select
connect 完成：select 可写≠成功；getsockopt(SO_ERROR)==0 才成功
listenfd + select：accept 必须非阻塞，防 RST 清空队列后阻塞死锁
```

---

## 与前面章节挂钩

| 章节 | 关联 |
|------|------|
| Ch 4 | connect、accept |
| Ch 6 | select 改造 str_cli → 16.2 终极版 |
| Ch 7 | fcntl、getsockopt SO_ERROR |
| Ch 14 | select 超时、MSG_DONTWAIT |
| Ch 14.9 | epoll 承接高并发非阻塞模型 |

---

## 阶段二（2_AdvancedSkill）进度

| 章 | 状态 |
|----|------|
| 11 名字与地址 | 厚版完成 |
| 13 守护进程/inetd | 厚版完成 |
| 14 高级 I/O | 厚版完成 |
| **16 非阻塞 I/O** | **厚版完成** |
| 26 线程 | 待笔记 |
