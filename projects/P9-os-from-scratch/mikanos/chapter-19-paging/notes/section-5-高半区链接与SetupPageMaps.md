## 5. 高半区链接与 SetupPageMaps

---

### 一、修改应用链接基址

**Makefile 应用侧：**

```makefile
CXXFLAGS += -mcmodel=large
LDFLAGS  += --image-base 0xffff800000000000
```

| 选项 | 作用 |
|------|------|
| **`--image-base 0xffff8000…`** | 符号与 **e_entry** 落在 **高半 canonical VA** |
| **`-mcmodel=large`** | 生成 **64 位绝对地址** 寻址指令 — 适配 **远 VA** |

**与内核低半/identity 区分离** — 链接器与 OS 约定一致。

---

### 二、加载 ELF LOAD 段

```
for each PT_LOAD:
    vaddr = ph.p_vaddr          # 已是 0xffff8000…
    memsz = ph.p_memsz
    copy file bytes → 新分配物理页
    SetupPageMaps(cr3, vaddr, memsz, phys_pages)
```

**`.bss`：** `p_memsz > p_filesz` 部分 **零填充物理页**。

---

### 三、SetupPageMaps() 递归

**按 VA 范围逐 **4KiB** 建表：**

```cpp
void SetupPageMaps(PageMap* pml4, uint64_t vaddr, size_t size, void* phys_base) {
    // 对每一页:
    //   确保 PML4→PDP→PD→PT 链存在（缺则 AllocatePage 填表项）
    //   PT[ idx1 ].SetPhysPage( phys_page, Present|Writable )
}
```

| 细节 | 说明 |
|------|------|
| **递归/分层** | 某级不存在 → **分配新表页** · **Present** |
| **物理页来源** | Ch8 **BitmapPageAllocator** |
| **CR3** | 指向 **应用 PML4 物理地址** |

**跳转前：** `LoadCr3(app_cr3)` · **`sti`**（Ch18）· `call e_entry(va)`。

---

### 四、与 Ch18 集成

**ExecuteLine → 加载 ELF 分支：**

```
旧: memcpy(load_phys); jump load_phys;
新: SetupPageMaps; cr3=app; jump e_entry; cr3=kernel; CleanPageMaps;
```

---

← [4. 四级分页](./section-4-x86-64四级分页.md) · 下一节 [6. 清理](./section-6-CleanPageMaps与小结.md)
