## ④ Shift 与 CapsLock · 符号与大小写

#### Shift + 双表

| 变量/表 | 作用 |
|---------|------|
| **`key_shift`** | 左/右 Shift **按下状态** |
| **`keytable0`** | 无 Shift |
| **`keytable1`** | 有 Shift → **`!` `%` 等** |

```
scancode → (shift ? keytable1 : keytable0)[code]
```

#### CapsLock + ASCII `0x20`

字母 **大小写 ASCII 差 `0x32`（即 0x20）**：

| 条件 | 转换 |
|------|------|
| CapsLock XOR Shift（组合规则以原书为准） | 在 **大小写间 toggle** |

**位运算/加减 `0x20`** — 比 **两套完整 A–Z 表** 省空间。

---
