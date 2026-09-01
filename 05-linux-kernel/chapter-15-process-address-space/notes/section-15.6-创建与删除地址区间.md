## ⑥ 创建与删除地址区间

用户态 **`mmap` / `munmap`** 最终进入内核 **`do_mmap` / `do_munmap`** — 操纵 **VMA 树** 与 **页表**（PTE 可延迟建立）。

> **本篇分工**：实体书已讲 `do_mmap` 的概念流程。本篇只做三件事：
> ① 用 v6.6 源码**逐段列出 `do_mmap()` / `mmap_region()` / `__do_munmap()` 的真实代码**
> （书的流程已经和现实对不上了）；
> ② **订正两个会出大事的误解** —— `MAP_FIXED` 的语义、`MAP_POPULATE` 的真实行为；
> ③ 补上书上没有的 `MAP_FIXED_NOREPLACE`、`MADV_POPULATE_WRITE`、munmap 的 detach 树。

---

## 1. ⚠️ 订正一：`do_mmap()` 的签名变了

书上的签名（6 参数，LKD3 时代）：
```c
/* ❌ 已经不是这个了 */
unsigned long do_mmap(struct file *file, unsigned long addr,
                      unsigned long len, unsigned long prot,
                      unsigned long flags, unsigned long offset);
```

**v6.6 的真实签名（9 参数）**：

```c
/* mm/mmap.c:1203 */
unsigned long do_mmap(struct file *file, unsigned long addr,
			unsigned long len, unsigned long prot,
			unsigned long flags, vm_flags_t vm_flags,
			unsigned long pgoff, unsigned long *populate,
			struct list_head *uf)
```

多出来的三个参数各有明确用途：

| 参数 | 用途 |
|------|------|
| **`vm_flags_t vm_flags`** | 让**内核内部调用者**（`vm_brk_flags`、`elf_map`、驱动）能直接传已经算好的 `VM_*` 标志，绕过 `prot`/`flags` 转换 |
| **`unsigned long *populate`** | **出参**。`do_mmap` 不自己做预取，它只告诉调用者"你回头要 populate 多少字节" |
| **`struct list_head *uf`** | userfaultfd 的事件链表（`MAP_FIXED` 需要 unmap 时收集事件） |

---

## 2. `do_mmap()` 逐段实证

### 2.1 参数合法性与 `READ_IMPLIES_EXEC`

```c
	struct mm_struct *mm = current->mm;
	int pkey = 0;

	*populate = 0;
	if (!len)
		return -EINVAL;

	/*
	 * Does the application expect PROT_READ to imply PROT_EXEC?
	 *
	 * (the exception is when the underlying filesystem is noexec
	 *  mounted, in which case we dont add PROT_EXEC.)
	 */
	if ((prot & PROT_READ) && (current->personality & READ_IMPLIES_EXEC))
		if (!(file && path_noexec(&file->f_path)))
			prot |= PROT_EXEC;

	/* force arch specific MAP_FIXED handling in get_unmapped_area */
	if (flags & MAP_FIXED_NOREPLACE)
		flags |= MAP_FIXED;

	if (!(flags & MAP_FIXED))
		addr = round_hint_to_min(addr);

	/* Careful about overflows.. */
	len = PAGE_ALIGN(len);
	if (!len)
		return -ENOMEM;

	/* offset overflow? */
	if ((pgoff + (len >> PAGE_SHIFT)) < pgoff)
		return -EOVERFLOW;

	/* Too many mappings? */
	if (mm->map_count > sysctl_max_map_count)
		return -ENOMEM;
```

| 检查 | 行为 |
|------|------|
| `len == 0` | `-EINVAL` |
| `READ_IMPLIES_EXEC` personality | `PROT_READ` 自动加 `PROT_EXEC`（**兼容老二进制**；`noexec` 挂载例外）|
| `len` 上溢 | `-ENOMEM` |
| `pgoff + pages` 溢出 | **`-EOVERFLOW`**（不是 `-EINVAL`）|
| `map_count > sysctl_max_map_count` | `-ENOMEM`。上限 = `DEFAULT_MAX_MAP_COUNT` = **`USHRT_MAX - 5` = 65530** |

⚠️ 注意 `MAP_FIXED_NOREPLACE` 的**实现方式**：先 `flags |= MAP_FIXED` 让
`get_unmapped_area()` 走"固定地址"分支，然后**再单独做一次重叠检查**（见 §3）。

### 2.2 取地址

```c
	addr = get_unmapped_area(file, addr, len, pgoff, flags);
	if (IS_ERR_VALUE(addr))
		return addr;
```

`mm->get_unmapped_area` 由 `arch_pick_mmap_layout()` 决定，x86_64 上是：

```c
/* arch/x86/mm/mmap.c:129 */
	if (mmap_is_legacy())
		mm->get_unmapped_area = arch_get_unmapped_area;          /* bottom-up，从低到高 */
	else
		mm->get_unmapped_area = arch_get_unmapped_area_topdown;  /* top-down，从 mmap_base 往下 */
```

默认是 **top-down**。legacy 模式（`personality(ADDR_COMPAT_LAYOUT)` 或 `sysctl_legacy_va_layout=1`）才是 bottom-up。

### 2.3 权限换算与 `VM_MAY*`

```c
	vm_flags |= calc_vm_prot_bits(prot, pkey) | calc_vm_flag_bits(flags) |
			mm->def_flags | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC;

	if (flags & MAP_LOCKED)
		if (!can_do_mlock())
			return -EPERM;

	if (!mlock_future_ok(mm, vm_flags, len))
		return -EAGAIN;
```

`calc_vm_prot_bits()` 的映射（`include/linux/mman.h`）：

| `mmap` 参数 | → `VM_*` |
|-------------|----------|
| `PROT_READ` | `VM_READ` |
| `PROT_WRITE` | `VM_WRITE` |
| `PROT_EXEC` | `VM_EXEC` |
| `MAP_GROWSDOWN` | `VM_GROWSDOWN` |
| `MAP_LOCKED` | `VM_LOCKED` |
| `MAP_SYNC` | `VM_SYNC` |

**同时无条件加上 `VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC`** ——
这三个是 `mprotect()` 的**上限**。这正是"先 `mmap(PROT_WRITE)` 再 `mprotect(PROT_READ)` 会失败"的根源：
`mmap` 时没给 `PROT_READ` 就没有 `VM_MAYREAD`（注意这里是无条件加的，
但后续 `MAP_PRIVATE` 分支会按 `f_mode` **清掉** `VM_MAYWRITE`/`VM_MAYSHARE`）。

`mm->def_flags` 也在这里被并入 —— `mlockall(MCL_FUTURE)` 就是靠把 `VM_LOCKED` 写进
`mm->def_flags` 来让**后续所有映射自动锁定**的。

### 2.4 文件映射的权限收敛（书上没有）

```c
	if (file) {
		switch (flags & MAP_TYPE) {
		case MAP_SHARED:
			/*
			 * Force use of MAP_SHARED_VALIDATE with non-legacy
			 * flags. E.g. MAP_SYNC is dangerous to use with
			 * MAP_SHARED as you don't know which consistency model
			 * you will get. We silently ignore unsupported flags
			 * with MAP_SHARED to preserve backward compatibility.
			 */
			flags &= LEGACY_MAP_MASK;
			fallthrough;
		case MAP_SHARED_VALIDATE:
			if (flags & ~flags_mask)
				return -EOPNOTSUPP;
			if (prot & PROT_WRITE) {
				if (!(file->f_mode & FMODE_WRITE))
					return -EACCES;
				if (IS_SWAPFILE(file->f_mapping->host))
					return -ETXTBSY;
			}
			/* Make sure we don't allow writing to an append-only file.. */
			if (IS_APPEND(inode) && (file->f_mode & FMODE_WRITE))
				return -EACCES;

			vm_flags |= VM_SHARED | VM_MAYSHARE;
			if (!(file->f_mode & FMODE_WRITE))
				vm_flags &= ~(VM_MAYWRITE | VM_SHARED);
			fallthrough;
		case MAP_PRIVATE:
			if (!(file->f_mode & FMODE_READ))
				return -EACCES;
			if (path_noexec(&file->f_path)) {
				if (vm_flags & VM_EXEC)
					return -EPERM;
				vm_flags &= ~VM_MAYEXEC;
			}
			if (!file->f_op->mmap)
				return -ENODEV;
			if (vm_flags & (VM_GROWSDOWN|VM_GROWSUP))
				return -EINVAL;
			break;
		default:
			return -EINVAL;
		}
	} else {
		...
	}
```

**`MAP_SHARED` 会静默丢弃非 legacy 标志**（`flags &= LEGACY_MAP_MASK`），
这是为了向后兼容。想要严格校验必须显式用 `MAP_SHARED_VALIDATE`，否则不支持的 flag 被静默忽略。

### 2.5 落地与 populate

```c
	addr = mmap_region(file, addr, len, vm_flags, pgoff, uf);
	if (!IS_ERR_VALUE(addr) &&
	    ((vm_flags & VM_LOCKED) ||
	     (flags & (MAP_POPULATE | MAP_NONBLOCK)) == MAP_POPULATE))
		*populate = len;
	return addr;
```

⚠️ **两个关键点**：

1. **`MAP_POPULATE | MAP_NONBLOCK` 组合会让 populate 失效**！
   判据是 `(flags & (MAP_POPULATE|MAP_NONBLOCK)) == MAP_POPULATE`，
   即**必须只有 `MAP_POPULATE`**。加了 `MAP_NONBLOCK` 就退化成异步预读（`MAP_NONBLOCK` 语义）。
2. `do_mmap()` **自己不预取**，它只把长度写进 `*populate`。
   真正的预取在系统调用入口 `ksys_mmap_pgoff()` 里做：
   ```c
   /* mm/mmap.c:2958 */
	unsigned long populate = 0;
	...
	ret = vm_mmap_pgoff(file, addr, len, prot, flags, pgoff, &populate, NULL);
	...
	if (populate)
		mm_populate(ret, populate);
   ```

---

## 3. ⚠️ 订正二：`MAP_FIXED` 会**静默覆盖**已有映射

`MAP_FIXED` 的语义是"**我就要这个地址，有东西就给我拆掉**"，不是"有东西就失败"。

```c
/* mm/mmap.c:1254 */
	if (flags & MAP_FIXED_NOREPLACE) {
		if (find_vma_intersection(mm, addr, addr + len))
			return -EEXIST;
	}
```

| flag | 目标地址已被占用时 | 引入版本 |
|------|------------------|---------|
| **`MAP_FIXED`** | **静默 unmap 旧的，装新的** | 古早 |
| **`MAP_FIXED_NOREPLACE`** | **返回 `-EEXIST`，什么都不做** | **v4.17** |

`mmap_region()` 里那句 `do_vmi_munmap(&vmi, mm, addr, len, uf, false)` 是无条件的
——**不管你有没有 `MAP_FIXED`，只要走到 `mmap_region` 就先拆**。
只是没有 `MAP_FIXED` 时 `get_unmapped_area()` 已经保证地址是空的，拆不到东西。

> **HFT 事故场景**：想在一块**预留好的固定地址**上装行情环，
> 用 `MAP_FIXED` 会**把恰好落在那里的 .so / 另一个 buffer 悄悄拆掉**。
> 现象是程序跑一段时间后在某个完全不相关的地方 SIGSEGV。
> **永远用 `MAP_FIXED_NOREPLACE`，并检查返回值不是 `-EEXIST`。**

`MAP_FIXED_NOREPLACE` 的开销：一次 `find_vma_intersection()`（= 一次 `mt_find()`），
只在 `mmap` 时发生一次，热路径无影响。

---

## 4. `mmap_region()`：合并优先的策略

```c
/* mm/mmap.c:2662 节选 */
	unsigned long mmap_region(struct file *file, unsigned long addr,
			unsigned long len, vm_flags_t vm_flags, unsigned long pgoff,
			struct list_head *uf)
{
	VMA_ITERATOR(vmi, mm, addr);

	/* ① 检查地址空间限额（RLIMIT_AS） */
	if (!may_expand_vm(mm, vm_flags, len >> PAGE_SHIFT)) {
		/* MAP_FIXED 会拆掉重叠的部分，先把它会释放的额度算进去 */
		nr_pages = count_vma_pages_range(mm, addr, end);
		if (!may_expand_vm(mm, vm_flags, (len >> PAGE_SHIFT) - nr_pages))
			return -ENOMEM;
	}

	/* ② 拆掉这个区间上的旧映射（MAP_FIXED 覆盖的实现就在这里） */
	if (do_vmi_munmap(&vmi, mm, addr, len, uf, false))
		return -ENOMEM;

	/* ③ 私有可写映射：overcommit 记账 */
	if (accountable_mapping(file, vm_flags)) {
		charged = len >> PAGE_SHIFT;
		if (security_vm_enough_memory_mm(mm, charged))
			return -ENOMEM;
		vm_flags |= VM_ACCOUNT;
	}

	/* ④ 找到前驱后继，尝试「扩展」已有 VMA */
	next = vma_next(&vmi);
	prev = vma_prev(&vmi);
	...
	if (vma && !vma_expand(&vmi, vma, merge_start, merge_end, vm_pgoff, next)) {
		khugepaged_enter_vma(vma, vm_flags);
		goto expanded;                       /* ★ 扩展成功，不新建 VMA */
	}

	/* ⑤ 扩展失败 → 新建 VMA */
cannot_expand:
	vma = vm_area_alloc(mm);
	...
	vma_iter_config(&vmi, addr, end);
	vma->vm_start = addr;
	vma->vm_end = end;
	vm_flags_init(vma, vm_flags);            /* ★ 注意是 _init，不是 _set */
	vma->vm_page_prot = vm_get_page_prot(vm_flags);
	vma->vm_pgoff = pgoff;

	if (file) {
		...
		error = call_mmap(file, vma);     /* 驱动的 ->mmap() 在这里被调用 */
		...
	} else if (vm_flags & VM_SHARED) {
		error = shmem_zero_setup(vma);   /* 匿名共享 → tmpfs 后备 */
	} else {
		vma_set_anonymous(vma);          /* 匿名私有 → vm_ops = NULL */
	}

	/* ⑥ MDWE 检查（PR_SET_MDWE） */
	if (map_deny_write_exec(vma, vma->vm_flags)) {
		error = -EACCES;
		goto close_and_free_vma;
	}

	/* ⑦ ★ 先给 maple 树预分配内存，再插入 */
	error = -ENOMEM;
	if (vma_iter_prealloc(&vmi, vma))
		goto close_and_free_vma;

	/* Lock the VMA since it is modified after insertion into VMA tree */
	vma_start_write(vma);
	vma_iter_store(&vmi, vma);               /* 插入 mm_mt */
	mm->map_count++;
	...
	khugepaged_enter_vma(vma, vma->vm_flags);
	...
	vm_stat_account(mm, vm_flags, len >> PAGE_SHIFT);
	if (vm_flags & VM_LOCKED) {
		if ((vm_flags & VM_SPECIAL) || vma_is_dax(vma) ||
				is_vm_hugetlb_page(vma) || vma == get_gate_vma(current->mm))
			vm_flags_clear(vma, VM_LOCKED_MASK);   /* 特殊 VMA 不能锁 */
		else
			mm->locked_vm += (len >> PAGE_SHIFT);
	}
```

### 4.1 为什么 `vma_iter_prealloc()` 必须在 `vma_iter_store()` 之前？

maple tree 插入一个区间可能需要**分配/分裂节点**。如果插入途中分配失败，
树会处于"更新了一半"的状态，回滚很麻烦。所以内核改成：

```
先 vma_iter_prealloc()   →  把可能需要的 maple 节点内存备好
再 vma_iter_store()      →  插入路径不再可能失败
```

这在 `do_brk_flags()` 里也一模一样（`vma_iter_prealloc` 失败就 `goto unacct_fail`）。

### 4.2 `VM_LOCKED` 会被清掉的四类 VMA

```c
	if ((vm_flags & VM_SPECIAL) || vma_is_dax(vma) ||
			 is_vm_hugetlb_page(vma) || vma == get_gate_vma(current->mm))
		vm_flags_clear(vma, VM_LOCKED_MASK);
```

| 类型 | 原因 |
|------|------|
| `VM_SPECIAL`（`VM_IO`/`VM_DONTEXPAND`/`VM_PFNMAP`/`VM_MIXEDMAP`） | 这些页**没有 `struct page`**，不在 LRU 上，本来就不会被换出，锁定无意义 |
| DAX | 持久内存的页走文件系统路径 |
| **hugetlb** | huge page **本来就不参与换出**，锁定无意义（但会占 `hugetlb_usage`） |
| `gate_vma`（vsyscall 页） | 内核固定映射 |

> **HFT 推论**：`MAP_HUGETLB` 的映射**不需要**再 `mlock` —— 大页本来就不换出。
> 反过来，THP 是**可以**被拆回 4KB 并换出的，**必须 `mlock`**（或 `MADV_NOHUGEPAGE` + mlock）。

---

## 5. `MAP_POPULATE` 的完整实现链（含一个坑）

```
syscall mmap(MAP_POPULATE)
  └─ ksys_mmap_pgoff()                          mm/mmap.c:2958
       └─ vm_mmap_pgoff(..., &populate, NULL)
            └─ do_mmap()                        → *populate = len
       └─ mm_populate(ret, populate)            mm/mmap.c:3026
            └─ __mm_populate(start, len, 0)     mm/gup.c:1737
                 └─ for each VMA:
                      populate_vma_page_range()  mm/gup.c:1646
                           └─ __get_user_pages(mm, start, nr_pages,
                                    gup_flags, NULL, locked)
```

```c
/* mm/gup.c:1646 */
long populate_vma_page_range(struct vm_area_struct *vma,
		unsigned long start, unsigned long end, int *locked)
{
	...
	/*
	 * Rightly or wrongly, the VM_LOCKONFAULT case has never used
	 * faultin_page() to break COW, so it has no work to do here.
	 */
	if (vma->vm_flags & VM_LOCKONFAULT)
		return nr_pages;                    /* ⚠️ 直接返回，什么都没做 */

	gup_flags = FOLL_TOUCH;
	/*
	 * We want to touch writable mappings with a write fault in order
	 * to break COW, except for shared mappings because these don't COW
	 * and we would not want to dirty them for nothing.
	 */
	if ((vma->vm_flags & (VM_WRITE | VM_SHARED)) == VM_WRITE)
		gup_flags |= FOLL_WRITE;            /* ★ 只给「私有可写」加 FOLL_WRITE */

	if (vma_is_accessible(vma))
		gup_flags |= FOLL_FORCE;
	if (locked)
		gup_flags |= FOLL_UNLOCKABLE;

	ret = __get_user_pages(mm, start, nr_pages, gup_flags, NULL, ...);
	lru_add_drain();
	return ret;
}
```

**⚠️ 坑：`VM_LOCKONFAULT` 会让 populate 变成空操作。**
如果进程之前调用过 `mlockall(MCL_CURRENT|MCL_ONFAULT)` 或 `mlock2(MLOCK_ONFAULT)`，
VMA 带 `VM_LOCKONFAULT`，`populate_vma_page_range()` **直接返回 `nr_pages` 而不真正填页**。
这是设计如此（ONFAULT 的语义就是"缺页时才锁"），但和 `MAP_POPULATE` 组合时
会得到"调用成功但页表没填"的结果。

**`FOLL_WRITE` 只给私有可写映射**：
- 私有可写 → 用**写缺页**填充，顺便把 COW 打断（这样后续写不再缺页）；
- 共享可写 → 只用**读缺页**，避免白白把共享页弄脏。

### 相比手写"遍历写一遍"

| 做法 | 页表 | 脏页 | 缺页次数 |
|------|------|------|---------|
| 手写 `for (i...) p[i] = 0;` | 填了 | **全部变脏**（`Dirty` 位置位） | 全部 |
| **`MADV_POPULATE_WRITE`** | 填了 | 全部变脏（等价于写缺页） | 全部 |
| **`MADV_POPULATE_READ`**（v5.14+） | 填了 | **不脏** | 全部 |
| **`MAP_POPULATE`** | 填了 | 私有映射会打断 COW；共享不脏 | 全部 |

> **HFT 建议**：启动阶段用 `MAP_POPULATE`（或 `mlockall(MCL_CURRENT)`），
> 中途扩容用 `madvise(MADV_POPULATE_WRITE)`。
> 如果只是想"预取但不想弄脏"（比如只读参考数据），用 `MADV_POPULATE_READ`。

---

## 6. `munmap`：`mt_detach` 侧树 + "不归点"

**v6.1 起 `__do_munmap()` 被彻底重写**（配合 maple tree）。新流程：

```c
/* mm/mmap.c:2462 起（节选） */
	struct maple_tree mt_detach;             /* ★ 临时侧树 */
	MA_STATE(mas_detach, &mt_detach, 0, 0);
	mt_init_flags(&mt_detach, vmi->mas.tree->ma_flags & MT_FLAGS_LOCK_MASK);
	mt_set_external_lock(&mt_detach, &mm->mmap_lock);

	/* ① 需要的话先劈开首尾 VMA */
	if (start > vma->vm_start) {
		error = __split_vma(vmi, vma, start, 1);
		...
	}
	...
	/* ② 逐个「摘」到侧树上，并标记 detached */
	for_each_vma_range(*vmi, next, end) {
		if (next->vm_end > end) {
			error = __split_vma(vmi, next, end, 0);
			...
		}
		vma_start_write(next);
		mas_set(&mas_detach, count);
		error = mas_store_gfp(&mas_detach, next, GFP_KERNEL);
		...
		vma_mark_detached(next, true);       /* ★ per-VMA lock 读者看到会 retry */
		if (next->vm_flags & VM_LOCKED)
			locked_vm += vma_pages(next);
		count++;
		...
	}

	/* ③ 从 mm_mt 里清掉整个区间 —— 此后这些 VMA 已经不可达 */
	error = vma_iter_clear_gfp(vmi, start, end, GFP_KERNEL);
	if (error)
		goto clear_tree_failed;

	/* Point of no return */
	mm->locked_vm -= locked_vm;
	mm->map_count -= count;
	if (unlock)
		mmap_write_downgrade(mm);           /* ★ 持写锁降级为读锁 */

	/* ④ 拆页表 + TLB flush（源码注释：因为 VMA 已隔离，无需写锁保护） */
	mas_set(&mas_detach, 1);
	unmap_region(mm, &mas_detach, vma, prev, next, start, end, count, !unlock);

	/* ⑤ 释放 VMA */
	mas_set(&mas_detach, 0);
	remove_mt(mm, &mas_detach);
	validate_mm(mm);
	if (unlock)
		mmap_read_unlock(mm);

	__mt_destroy(&mt_detach);
	return 0;
```

### 关键设计点

| 步骤 | 为什么这么做 |
|------|-------------|
| **② 先标记 `detached`** | per-VMA lock 的读者（缺页快路径）看到 `detached` 会 `retry`，**不会拿到一个正在被拆的 VMA** |
| **③ 先清 `mm_mt` 再拆页表** | 一旦从树上摘掉，地址查询立刻返回"没有 VMA"→ 新来的访问走 `bad_area` → SIGSEGV，而不是访问正在被拆的页表 |
| **④ 锁降级为读锁** | 拆页表 + `flush_tlb_range` 可能很慢（几十 MB 的区间）。**在只读锁下做**，其他核的缺页可以继续并发进行 |
| **"Point of no return"** | 过了这行就**没有错误返回路径**了。所以所有可能失败的分配（② 的 `mas_store_gfp`）都在之前做完 |

**版本对照**：

| 版本 | VMA 摘除方式 |
|------|-------------|
| v6.0 | `detach_vmas_to_be_unmapped()` 移到**链表**；已经有锁降级 |
| **v6.1** | 改为移到 **`mt_detach` maple 侧树**；配合 `mas_store_gfp` 预分配 |
| v6.6 | 同上，加 `vma_mark_detached()` 配合 per-VMA lock |

### munmap 的副作用一览

| 副作用 | 说明 |
|--------|------|
| **文件映射** | `fput(vma->vm_file)`、从 `i_mmap` interval tree 摘除 |
| **共享匿名** | 只有**最后一个**映射者 `munmap` 后 shmem inode 才释放 |
| **TLB** | `unmap_region()` → `zap_page_range()` → `tlb_finish_mmu()` 里批量 flush |
| **VMA 数** | `mm->map_count -= count` |
| **`locked_vm`** | 扣掉被解除的 `VM_LOCKED` 页数 |

---

## 7. `brk` / `sbrk` 的另一条路

`malloc` 小内存走 `brk`，大内存走 `mmap`。`brk` 的内核路径与 `mmap` **完全独立**：

```c
SYSCALL_DEFINE1(brk, unsigned long brk)     /* mm/mmap.c:177 */
  └─ 检查 min_brk / RLIMIT_DATA / 是否与已有 VMA 冲突
  └─ 检查下一个 VMA 是否在 stack_guard_gap 之内（mm/mmap.c:253）
  └─ do_brk_flags(&vmi, brkvma, oldbrk, newbrk - oldbrk, 0)
       ├─ may_expand_vm()   → RLIMIT_AS 检查
       ├─ map_count 检查
       ├─ security_vm_enough_memory_mm()
       ├─ 【优先扩展已有 brk VMA】can_vma_merge_after() → vma_expand
       └─ 【否则】vm_area_alloc + vma_set_anonymous + vma_iter_store_gfp
  └─ if (populate) mm_populate(oldbrk, newbrk - oldbrk);
```

```c
/* mm/mmap.c:2142 —— brk 收缩时把 VMA 整个删掉 */
unsigned long stack_guard_gap = 256UL<<PAGE_SHIFT;   /* = 1 MB */
```

⚠️ **`brk` 的 VMA 是 `VM_GROWSUP`**，不是 `VM_GROWSDOWN`（栈才是 GROWSDOWN）。
`do_brk_flags()` 里 `flags |= VM_DATA_DEFAULT_FLAGS | VM_ACCOUNT | mm->def_flags`。

---

## 8. `madvise()` 全家桶（HFT 相关）

```c
/* mm/madvise.c:1034 主分支 */
	case MADV_DONTNEED:
	case MADV_DONTNEED_LOCKED:
	case MADV_FREE:      → madvise_dontneed_free()   /* 拆页表，下次访问重新缺页 */
	case MADV_POPULATE_READ:
	case MADV_POPULATE_WRITE: → madvise_populate()   /* 填页表 */
	case MADV_COLLAPSE:  → madvise_collapse()        /* 同步折叠成 THP */
	case MADV_DONTFORK:  → new_flags |= VM_DONTCOPY
	case MADV_WIPEONFORK:→ new_flags |= VM_WIPEONFORK
	case MADV_DONTDUMP:  → new_flags |= VM_DONTDUMP
	case MADV_HUGEPAGE / MADV_NOHUGEPAGE → VM_HUGEPAGE / VM_NOHUGEPAGE
```

| `madvise` | 值 | HFT 用法 | 注意 |
|-----------|-----|---------|------|
| **`MADV_DONTFORK`** | 10 | 防止 `fork` 时子进程复制这块内存（行情环、大 buffer） | 置 `VM_DONTCOPY` |
| **`MADV_WIPEONFORK`** | 18 | `fork` 时子进程看到的是**清零**的新内存（防密钥/状态泄漏） | **只支持私有匿名**（有 `vm_file` 或 `VM_SHARED` → `-EINVAL`） |
| **`MADV_DONTDUMP`** | 16 | 从 core dump 排除大 buffer（core 文件小几十 GB） | 置 `VM_DONTDUMP` |
| **`MADV_HUGEPAGE`** | 14 | 标记这个 VMA 是 THP 候选 | 只影响 THP，**不影响** hugetlb |
| **`MADV_NOHUGEPAGE`** | 15 | 标记这个 VMA **不要** THP | HFT 常用 |
| **`MADV_POPULATE_READ`** | 22 | 预取页表，**不弄脏** | v5.14+ |
| **`MADV_POPULATE_WRITE`** | 23 | 预取页表 + 打断 COW | v5.14+ |
| **`MADV_DONTNEED_LOCKED`** | 24 | 连 `mlock` 的页也一起释放 | v5.18+；普通 `MADV_DONTNEED` 对 `VM_LOCKED` 页无效 |
| **`MADV_COLLAPSE`** | 25 | **同步**把这段折叠成 THP（不等 khugepaged） | v6.1+ |
| **`MADV_FREE`** | 8 | 惰性释放：内存压力时才回收 | 匿名页专用 |

⚠️ **`madvise` 可能劈开 VMA**：`madvise_update_vma()` 在 flags 变化时会
`vma_modify()` → 可能 split。对**子区间**反复 `madvise` 会让 VMA 数暴涨，
逼近 65530 上限。

---

## 9. overcommit 与资源限额

| 检查 | 作用 | 相关旋钮 |
|------|------|---------|
| `may_expand_vm()` | **RLIMIT_AS**（虚拟内存总量） | `ulimit -v` |
| `security_vm_enough_memory_mm()` | **overcommit** 记账 | `/proc/sys/vm/overcommit_memory`（0=启发式 / 1=总是 / 2=严格） |
| `mlock_future_ok()` | **RLIMIT_MEMLOCK** | `ulimit -l` |
| `can_do_mlock()` | `RLIMIT_MEMLOCK != 0` 或 `CAP_IPC_LOCK` | — |
| `mm->map_count > sysctl_max_map_count` | VMA 个数 | `/proc/sys/vm/max_map_count`（默认 **65530**） |

`MAP_NORESERVE` 的作用（源码）：

```c
		/* We honor MAP_NORESERVE if allowed to overcommit */
		if (sysctl_overcommit_memory != OVERCOMMIT_NEVER)
			vm_flags |= VM_NORESERVE;
		/* hugetlb applies strict overcommit unless MAP_NORESERVE */
		if (file && is_file_hugepages(file))
			vm_flags |= VM_NORESERVE;
```

> **HFT 建议**：交易机上把 `vm.overcommit_memory` 保持默认（0，启发式）或设成 **2（严格）**，
> 并且**给进程配足够大的 `RLIMIT_AS`**。严格模式下 `mmap` 会立刻失败而不是等到缺页时 OOM。

---

## 10. HFT：启动脚本清单与盘中纪律

```
启动阶段（一次性成本，随便慢）：
  1. sysctl vm.nr_hugepages=<N>                  ← 先备好 huge page pool
  2. mmap(MAP_HUGETLB | MAP_SHARED | MAP_POPULATE | MAP_FIXED_NOREPLACE)
                                                  ← 用 NOREPLACE！
  3. mlockall(MCL_CURRENT | MCL_FUTURE)           ← 不换出 + 绕开回收路径
  4. madvise(ring, size, MADV_DONTFORK)           ← 防 fork 复制
  5. madvise(scratch, size, MADV_DONTDUMP)        ← core 文件别带上几十 GB
  6. prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, ring, size, "orderbook")
  7. 遍历一遍所有热数据结构（warm up）            ← 把 TLB / cache 也预热

盘中纪律（零分配、零 munmap）：
  - 不 mmap / 不 munmap / 不 brk（VMA 树只走读路径）
  - 不 fork（COW 尖刺）
  - 扩容只在低位波动时段做，用 MADV_POPULATE_WRITE 预热
  - 监控：grep -c '' /proc/self/maps  （VMA 数，防合并失败导致暴涨）
           grep VmLck /proc/self/status
```

**`MAP_FIXED` vs `MAP_FIXED_NOREPLACE` 决策表**：

| 场景 | 用哪个 |
|------|--------|
| 只想拿一块地址，位置无所谓 | **都不要用**，`addr = NULL` |
| 想要某块特定地址，且**确定**那里是空的 | **`MAP_FIXED_NOREPLACE`**（并检查 `-EEXIST`） |
| 想**替换**某块地址上的旧映射 | `MAP_FIXED` |
| 想做"预留一段大 VA，之后逐步提交"的 two-stage 映射 | 先 `PROT_NONE` 预留，再用 **`MAP_FIXED`**（语义就是替换）或 `mprotect` 提交 |

---

→ [Ch 5 syscall](../../chapter-05-system-calls/) · [Ch 16 页缓存](../../chapter-16-page-cache/) · [Ch 15.3 VMA](./section-15.3-虚拟内存区域.md) · [01 CSAPP mmap](../../../02-computer-systems/chapter-09-virtual-memory/) · [14 HFT Practice](../../../14-hft-engineering/)


<details>
<summary>自测题（点击展开）</summary>

**Q1.** mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, -1, 0) 做了什么？

<details><summary>答案</summary>

1) do_mmap 在 mm 中创建新 VMA（VM_READ|VM_WRITE|VM_SHARED|VM_ANONYMOUS）；2) 不分配物理页（延迟到首次访问）；3) 返回 VMA 起始地址。首次写 → page fault → 分配物理页 → 建 PTE。MAP_SHARED 的页在 fork 后父子共享（可用于 IPC）。HFT 用 MAP_SHARED|MAP_LOCKED 共享行情数据并锁在物理内存。

**按 v6.6 修订/补充**：

1. **匿名 `MAP_SHARED` 会走 `shmem_zero_setup()`**，挂上 `shmem_vm_ops`
   → `vma_is_anonymous()` 是 **false**。数据页是 **tmpfs/shmem 页**，走页缓存，
   **可以 swap**（除非 mlock），计入 `RssShmem` 而非 `RssAnon`。
2. **`mmap_region()` 会先尝试合并/扩展相邻 VMA**（`vma_expand()`），
   不成功才 `vm_area_alloc()` 新建。所以可能**没有新建 VMA**。
3. **插入顺序有讲究**：`vma_iter_prealloc()`（给 maple 树预分配节点）
   → `vma_start_write()` → `vma_iter_store()`。预分配是为了让插入**不可能失败**。
4. 首次访问时还有 **fault-around**：`vm_ops->map_pages`（文件映射是 `filemap_map_pages`）
   会顺带把相邻若干页一起填进 PTE。匿名映射没有 `map_pages`，一次只填一页。
5. **HFT 用的 `MAP_SHARED|MAP_LOCKED` 必须同时 `mlock`**：`MAP_LOCKED` 只置 `VM_LOCKED` 标志
   并把 `locked_vm` 记账加上，**并不立刻填页**；真正填页靠 `MAP_POPULATE` 或后续缺页。

</details>


**Q2.** mlock() 对 HFT 有什么意义？

<details><summary>答案</summary>

mlock 锁定内存页在物理 RAM 中，禁止换出到 swap。HFT 交易数据如果被换出，访问时需要磁盘 IO → 毫秒级延迟 → 灾难。mlockall(MCL_CURRENT|MCL_FUTURE) 锁定当前和未来所有页。HFT 进程启动时 mlockall 防止任何页被换出。需要 CAP_IPC_LOCK 或 root 权限。

**按 v6.6 补充**：

1. **"需要 CAP_IPC_LOCK 或 root"不完全对**。判据是：
   ```c
   bool can_do_mlock(void)
   {
       if (rlimit(RLIMIT_MEMLOCK) != 0) return true;   /* ← ulimit -l 非 0 就行 */
       if (capable(CAP_IPC_LOCK)) return true;
       return false;
   }
   ```
   即**只要 `RLIMIT_MEMLOCK != 0`（`ulimit -l` 是有限值，哪怕是 64KB）就允许调用**，
   只是超过额度的部分会被拒（`mlock_future_ok()` 返回 false → `-EAGAIN`）。
   `CAP_IPC_LOCK` 的作用是完全绕开额度检查：
   ```c
   bool mlock_future_ok(struct mm_struct *mm, unsigned long flags, unsigned long bytes)
   {
       if (!(flags & VM_LOCKED) || capable(CAP_IPC_LOCK))
           return true;
       ...
   }
   ```
2. **`mlockall(MCL_FUTURE)` 的实现**是 `mm->def_flags |= VM_LOCKED`
   （`apply_mlockall_flags()`，`mm/mlock.c:667`），
   而 `do_mmap()` 里有 `vm_flags |= ... | mm->def_flags | ...`，
   所以**后续所有新映射自动带 `VM_LOCKED`**。
3. **四类 VMA 会被强制清掉 `VM_LOCKED`**（`mmap_region()` 里 `vm_flags_clear(vma, VM_LOCKED_MASK)`）：
   `VM_SPECIAL`（IO/PFNMAP/MIXEDMAP/DONTEXPAND）、DAX、**hugetlb**、`gate_vma`。
   → **`MAP_HUGETLB` 的映射本来就不换出，不需要再 mlock**。
4. ⚠️ **`mlockall(MCL_ONFAULT)` 与 `MAP_POPULATE` 冲突**：
   `VM_LOCKONFAULT` 会让 `populate_vma_page_range()` 直接 `return nr_pages`
   **而不真正填页**（`mm/gup.c:1658`）。ONFAULT 的语义就是"缺页时才锁"。
5. **mlock 的第二个收益（常被忽略）**：锁定页**不在 LRU 上、不参与回收**，
   于是 `try_to_unmap()` 不再遍历它的 rmap 链 → 消掉回收路径的锁竞争和 TLB shootdown IPI。

</details>


**Q3.** `MAP_FIXED` 和 `MAP_FIXED_NOREPLACE` 有什么区别？我该用哪个？

<details><summary>答案</summary>

| flag | 目标地址已被占用时 | 引入版本 |
|------|------------------|---------|
| **`MAP_FIXED`** | **静默 unmap 旧映射，装新的** | 古早 |
| **`MAP_FIXED_NOREPLACE`** | **返回 `-EEXIST`，什么都不改** | **v4.17** |

`MAP_FIXED` 的覆盖实现在 `mmap_region()` 里，是**无条件**的：
```c
	/* Unmap any existing mapping in the area */
	if (do_vmi_munmap(&vmi, mm, addr, len, uf, false))
		return -ENOMEM;
```
只是没有 `MAP_FIXED` 时 `get_unmapped_area()` 已经保证地址空闲，拆不到东西。

`MAP_FIXED_NOREPLACE` 的实现（`mm/mmap.c:1228` 和 `:1254`）：
```c
	/* force arch specific MAP_FIXED handling in get_unmapped_area */
	if (flags & MAP_FIXED_NOREPLACE)
		flags |= MAP_FIXED;              /* 先借用 MAP_FIXED 的地址选择逻辑 */
	...
	if (flags & MAP_FIXED_NOREPLACE) {
		if (find_vma_intersection(mm, addr, addr + len))
			return -EEXIST;              /* 再单独做一次重叠检查 */
	}
```

**该用哪个**：

| 场景 | 用哪个 |
|------|--------|
| 位置无所谓 | 都别用，`addr = NULL` |
| 想要特定地址且**确定**空闲 | **`MAP_FIXED_NOREPLACE`** + 检查 `-EEXIST` |
| 想**替换**上面的旧映射 | `MAP_FIXED` |
| two-stage（先 `PROT_NONE` 预留，后逐步提交） | 提交阶段用 `MAP_FIXED` 或 `mprotect` |

**典型事故**：用 `MAP_FIXED` 在一块以为空闲的地址上装 buffer，
结果那里其实有一个 .so 或另一个 thread 的栈 → **被静默拆掉** →
程序跑一段时间后在完全不相关的地方 SIGSEGV，且 `/proc/pid/maps` 上已经看不出来了。

开销：`MAP_FIXED_NOREPLACE` 只多一次 `find_vma_intersection()`（一次 `mt_find()`），
只在 `mmap` 时发生一次。

</details>


**Q4.** `MAP_POPULATE` 到底是怎么把页填进去的？用它会有什么意外？

<details><summary>答案</summary>

完整调用链：

```
syscall mmap(MAP_POPULATE)
  └─ ksys_mmap_pgoff()                        mm/mmap.c:2958
       └─ vm_mmap_pgoff(..., &populate, NULL)
            └─ do_mmap()  →  *populate = len
       └─ if (populate) mm_populate(ret, populate)
            └─ __mm_populate(start, len, 0)   mm/gup.c:1737
                 └─ 逐 VMA：populate_vma_page_range()   mm/gup.c:1646
                      └─ __get_user_pages(..., gup_flags, NULL, locked)
                           → 走 GUP，触发缺页把 PTE 填上
```

`do_mmap()` 自己**不做**预取，它只把长度写进出参 `*populate`：
```c
	addr = mmap_region(file, addr, len, vm_flags, pgoff, uf);
	if (!IS_ERR_VALUE(addr) &&
	    ((vm_flags & VM_LOCKED) ||
	     (flags & (MAP_POPULATE | MAP_NONBLOCK)) == MAP_POPULATE))
		*populate = len;
```

**三个意外**：

1. ⚠️ **`MAP_POPULATE | MAP_NONBLOCK` 会让 populate 完全失效**。
   判据要求 `(flags & (MAP_POPULATE|MAP_NONBLOCK)) == MAP_POPULATE`，
   即**只能有 `MAP_POPULATE`**。加了 `MAP_NONBLOCK` 就退化成异步预读。
2. ⚠️ **`VM_LOCKONFAULT` 让它是空操作**。
   `populate_vma_page_range()` 开头：
   ```c
   if (vma->vm_flags & VM_LOCKONFAULT)
       return nr_pages;      /* 什么都不做，直接返回成功 */
   ```
   进程若用过 `mlockall(MCL_ONFAULT)` 或 `mlock2(MLOCK_ONFAULT)`，
   `MAP_POPULATE` 会"成功但没填页"。
3. **`FOLL_WRITE` 只给私有可写映射**：
   ```c
   if ((vma->vm_flags & (VM_WRITE | VM_SHARED)) == VM_WRITE)
       gup_flags |= FOLL_WRITE;
   ```
   私有可写 → 用写缺页填充，**顺便打断 COW**；
   共享可写 → 只用读缺页，**避免白白弄脏共享页**。

**替代方案**：

| 需求 | 用什么 |
|------|--------|
| 启动阶段预取 | `MAP_POPULATE`（或 `mlockall(MCL_CURRENT)`） |
| 中途扩容后预取 | `madvise(MADV_POPULATE_WRITE)`（v5.14+） |
| 预取但**不想弄脏** | `madvise(MADV_POPULATE_READ)`（v5.14+） |
| 预取且要折叠成 THP | `madvise(MADV_COLLAPSE)`（v6.1+） |

</details>


**Q5.** `munmap` 在 v6.6 里是怎么做的？为什么中间有个"不归点"？

<details><summary>答案</summary>

v6.1 起 `__do_munmap()` 被完全重写（配合 maple tree），核心是**一棵临时侧树 `mt_detach`**：

```c
	struct maple_tree mt_detach;
	MA_STATE(mas_detach, &mt_detach, 0, 0);
	mt_init_flags(&mt_detach, vmi->mas.tree->ma_flags & MT_FLAGS_LOCK_MASK);

	/* ① 需要时先劈开首尾 VMA */
	__split_vma(vmi, vma, start, 1);   /   __split_vma(vmi, next, end, 0);

	/* ② 逐个「摘」到侧树，并标记 detached */
	for_each_vma_range(*vmi, next, end) {
		vma_start_write(next);
		mas_store_gfp(&mas_detach, next, GFP_KERNEL);   /* 可能失败的分配在这里 */
		vma_mark_detached(next, true);                  /* per-VMA lock 读者看到会 retry */
		count++;
	}

	/* ③ 从 mm_mt 清掉整个区间 —— 此后这些 VMA 已不可达 */
	vma_iter_clear_gfp(vmi, start, end, GFP_KERNEL);

	/* Point of no return —— 之后没有错误返回路径 */
	mm->locked_vm -= locked_vm;
	mm->map_count -= count;
	if (unlock)
		mmap_write_downgrade(mm);                       /* 写锁降级为读锁 */

	/* ④ 拆页表 + TLB flush（只读锁下做，其他核的缺页可并发） */
	unmap_region(mm, &mas_detach, vma, prev, next, start, end, count, !unlock);

	/* ⑤ 释放 VMA */
	remove_mt(mm, &mas_detach);
	__mt_destroy(&mt_detach);
```

**每一步的理由**：

| 步骤 | 为什么 |
|------|--------|
| ② 先标 `detached` | 缺页快路径（per-VMA lock）看到 `detached` 会 `retry`，不会拿到正在被拆的 VMA |
| ② 的分配都在前面 | 所有可能失败的 `GFP_KERNEL` 分配**必须在"不归点"之前完成** |
| ③ 先摘树再拆页表 | 摘掉之后地址查询立刻返回"无 VMA" → 新访问直接 SIGSEGV，不会碰正在拆的页表 |
| ④ 锁降级 | 拆页表 + flush TLB 可能很慢（几十 MB 区间）。在**读锁**下做，其他核的缺页可并发 |
| "不归点" | 过了这行就没有回滚路径了，所以前面必须把所有可能失败的事做完 |

**版本对照**：v6.0 用链表 `detach_vmas_to_be_unmapped()`（已有锁降级）；
**v6.1 改为 maple 侧树 `mt_detach`**；v6.6 再加 `vma_mark_detached()` 配合 per-VMA lock。

**HFT 推论**：`munmap` 的持写锁时间 = 步骤 ①②③（很短，主要是树操作）；
拆页表和 TLB flush 在**读锁**下完成。
但 **`munmap` 仍会拿写锁**，且 `mmap_write_downgrade()` 之后的读锁会一直持到函数结束。
→ 盘中做 `munmap` 依然会阻塞所有并发 `mmap`/`mprotect`，**但可以和其他核的缺页并发**。

</details>

</details>
---
