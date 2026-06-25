## ④ INT 0x0d · 一般保护异常

#### 何时触发

| 违规行为 | 结果 |
|----------|------|
| **越界访问** 受限段 | CPU **`#GP` → INT 0x0d** |
| app 执行 **`CLI` / `HLT` / LGDT…** | 特权指令 → **0x0d** |
| **`MOV DS, AX`** 加载 **内核段选择子** | 权限不足 → **0x0d** |

#### `inthandler0d`

捕获后：

1. **强制结束** 当前 app  
2. **打印 EIP 等寄存器** — **调试友好**  
3. **回到 Console** — OS **不死**

#### 段属性 **`+0x60`（DPL 示意）**

GDT 里 app 段 **限低特权（Ring 3 / DPL=3）** — app **不能** 加载 **OS 段到 DS/CS**。

**唯一合法出口：** **`INT 0x40`** — API **受控网关**。

```
app (低权限, 1003/1004)
    正常: INT 0x40 → OS 服务
    使坏: 写内核 / CLI / 改 DS → #GP → inthandler0d → 杀 app
```

**里程碑：** 「纸娃娃」→ **能扛 crack + bug** 的 **最小保护 OS**。

→ [01-CSAPP Ch8 异常](../../../../01-CSAPP-3rd/chapter-08-exceptional-control-flow/) · [05-LKD 缺页/GPF](../../../../05-Linux-Kernel-Development/)

---
