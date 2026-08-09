# Ch 1 §3 浏览代码 (Browsing the Code)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 3. 浏览代码 (Browsing the Code)

内核函数常 **跨文件、跨 `arch/`**，纯文本搜索容易迷路。原书推荐：

| 工具 | 做什么 | 今日常用替代 |
|------|--------|--------------|
| **LXR** (Linux Cross Referencing) | Web 上浏览源码；标识符 **超链接** 到定义/引用 | [elixir.bootlin.com](https://elixir.bootlin.com/linux/latest/source) · IDE **LSP/clangd** · `cscope`/`ctags` |
| **CodeViz**（作者为写书开发） | 生成 **函数调用图 (call graph)**，一眼看子系统结构 | **Doxygen Graph** · **CodeCompass** · 手工 `grep` + 画图 |

**第一次读 `mm/`：** 先能在浏览器里 **点函数名跳转**，比硬啃 `vim` 省一半时间。

---
