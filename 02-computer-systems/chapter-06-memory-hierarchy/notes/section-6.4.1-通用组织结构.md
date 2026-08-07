## 6.4.1 通用组织结构

> **Ch6 §6.4.1** · [章导读](../README.md) · 上节 [§6.3 ←](./section-6.3-层次结构与缓存概念.md) · 下节 [§6.4.2 →](./section-6.4.2-直接映射.md)

---

地址划分为（从低位到高位）：

```
| block offset (b) | set index (s) | tag (t) |
```

- **S = 2^s** 组，每组 **E** 条 cache line，每条 **B = 2^b** 字节
- 查 cache：**index 选组 → tag 比较 → valid 位**

---

### 常见陷阱

1. **混淆地址划分的位域顺序** — 从低位到高位是 **block offset (b) → set index (s) → tag (t)**。offset 在最低位因为它是块内偏移；index 在中间用于选组；tag 在最高位用于区分同组的不同块。
2. **以为 cache 查找是「先搜所有行」** — 实际是**三步**：①index 选组（直接寻址，不搜索）→ ②tag 比较（只在组内 E 路比较）→ ③valid 位检查。不是暴力搜索所有行。
3. **忘了 valid 位** — 即使 tag 匹配，如果 valid 位为 0，仍然 miss。开机时所有 cache line 的 valid 位都是 0。

### 自测题

<details>
<summary>1. 物理地址如何划分为 cache 查找所需的三个字段？</summary>

从低位到高位：**block offset (b 位)** — 块内偏移，寻址 64B 内的字节；**set index (s 位)** — 选哪一组；**tag (t 位)** — 区分同组内的不同块。t = 地址总位数 - s - b。
</details>

<details>
<summary>2. cache 查找的三步是什么？</summary>

①**index 选组**：用 set index 直接定位到某一组（不是搜索）；②**tag 比较**：在该组的 E 条 line 中比较 tag 字段；③**valid 检查**：tag 匹配且 valid=1 才命中。三步都通过才算 hit。
</details>

<details>
<summary>3. S、E、B 三个参数分别决定什么？容量怎么算？</summary>

**S = 2^s**（组数）、**E**（每组的 cache line 数，即相联度）、**B = 2^b**（每条 line 的字节数）。容量 ≈ S × E × B。例如 32KB L1：S=64, E=8, B=64 → 64×8×64 = 32768 = 32KB。
</details>

---

← [§6.3 ←](./section-6.3-层次结构与缓存概念.md) · [本章导读](../README.md) · [§6.4.2 →](./section-6.4.2-直接映射.md)
