## ① 攻克字符串 API · 段基址

Day 20 **EDX=2/3 字符串 API** 失败 — **只乱码或崩溃**。

#### 根因

OS API 用 **当前 DS** 解引用 app 传来的 **指针** — 但 **DS 仍指向内核数据段** → 从 **错误线性地址** 读字符串。

#### 修复

启动 app 时把 **app 数据段基址** 存约定位置（原书 **`0xfe8`**）：

```
API 读字符串:
    base = *(app_data_base_at_0xfe8)
    物理/线性地址 = base + app_传来的_offset
```

**教训：** **跨特权传指针** 必须 **显式指定目标地址空间** — Linux **copy_from_user** 同源。

→ [Day 5 GDT/段](../day-05-gdt-idt/) · [Day 20 INT 0x40](../day-20-api/)

---
