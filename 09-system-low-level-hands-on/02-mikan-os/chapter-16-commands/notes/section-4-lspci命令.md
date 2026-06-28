## 4. lspci 命令

---

### 一、展示底层能力

**类似 Linux `lspci`** — 列出 **PCI 总线设备** — 证明 OS **能调用 Ch6 驱动数据**。

---

### 二、pci::devices 数组

**Ch6 枚举时注册：**

```cpp
namespace pci {
    struct Device { uint8_t bus, device, func; uint16_t vendor; uint8_t class_code; … };
    extern std::vector<Device> devices;  // 或固定数组
}
```

**lspci 命令：**

```cpp
for (auto& d : pci::devices) {
    Print("bus %02x:%02x.%x class %02x vendor …", …);
}
```

| 输出字段（示意） | 来源 |
|------------------|------|
| **bus / dev / func** | 配置空间枚举 |
| **Vendor / Class** | Ch6 **CONFIG_ADDRESS** 读取 |

→ [Ch6 PCI 枚举](../chapter-06-mouse-pci/notes/section-4-PCI配置空间与枚举.md)

---

### 三、CLI 与内核边界

| 层 | lspci 所在 |
|----|------------|
| **Terminal / TaskTerminal** | 用户 **打字** |
| **命令 handler** | **内核态** 直接读 **pci::devices** |

**尚无 syscall** — 命令 **编译进内核**；Ch20 后 **用户态应用** 需 **接口**。

→ [chapter-20-syscall](../chapter-20-syscall/)

---

← [3. echo/clear](./section-3-echo与clear命令.md) · 下一节 [5. 历史](./section-5-命令历史与方向键.md)
