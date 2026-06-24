## ① 第一个 API · 优雅结束程序

#### OS 的价值

应用 **不应** 自己画每个像素 — 应 **调用 OS 提供的服务**。

#### 初版：绝对地址 + CALL

| 做法 | 问题 |
|------|------|
| app 里 **写死 OS 函数物理/线性地址** | **CALL** 跳转 |

能 **在 Console 显示一个字符** — 但 **脆弱**（Day ② 详述）。

#### 程序结束 · 不再 HLT 卡死

Day 19 **`hlt.hrb`** 结束 → **HLT 死循环** → **Shell 再也收不到输入**。

| 机制 | 方向 |
|------|------|
| OS 用 **`far-CALL`** 启动 app | OS → app |
| app 结尾 **`RETF`（远返回）** | app → **回到 OS Console** |

```
Console → far-CALL app段:入口
              app 运行 …
              RETF → 继续 Shell 读命令
```

→ [Day 19 farjmp](../day-19-apps/) · [Day 6 IRETD/栈](../day-06-split-compile-irq/)

---
