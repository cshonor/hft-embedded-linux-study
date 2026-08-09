## 7.8 可执行目标文件

> **Ch7 §7.8** · [章导读](../README.md) · 上节 [§7.7 ←](./section-7.7-重定位.md) · 下节 [§7.9 →](./section-7.9-加载可执行目标文件.md)

---

← [本章导读](../README.md)

---

### 口述巩固 · 自测

1. （待口述补）本节核心一句话？

### 自测题

<details>
<summary>1. 可执行目标文件和可重定位目标文件有什么区别？</summary>

可执行文件：1. 有**程序头表(Program Header Table)**——告诉 OS 如何加载各 segment 到内存
2. 地址已确定（已重定位）
3. 有 `.init`/`.fini` section（C 运行时初始化/终止代码）
4. 有入口点地址（`_start`，不是 `main`）

可重定位文件没有程序头表，地址未确定，不能直接执行。`readelf -l` 看程序头，`readelf -h` 看入口点。

</details>


---

← [§7.7 ←](./section-7.7-重定位.md) · [本章导读](../README.md) · [§7.9 →](./section-7.9-加载可执行目标文件.md)
