# demo/12-list/ —— 内核链表机制全量实测

对应 [4.12 container_of 与侵入式链表](../../4.12-container-of-list/4.12-container_of与侵入式链表.md)。

## 运行

```bash
# 在 WSL 中 (注意: 用 wsl -d Ubuntu --exec 调用, 不要用 wsl -- bash -lc)
wsl -d Ubuntu --exec bash -c "cd /mnt/d/repos/hft/01-c-language/05-embedded-kernel-practice/04-ch4-kernel-module/demo/12-list && bash run.sh"
```

## 测试清单

| 文件 | 主题 | 关键结论 |
|------|------|----------|
| `t1_invasive_vs_external.c` | 侵入式 vs 外挂式 | 侵入式零分配、cache 友好、一结构体挂多链表 |
| `t2_multi_list.c` | 一个结构体挂两张链表 | 同一份宏, member 参数不同, 回推偏移不同 |
| `t3_list_ops.c` | list_del POISON vs list_del_init | 投毒快速暴露 use-after-del; del_init 可安全重用 |
| `t4_iter_macros.c` | 遍历宏家族 | safe 版预存下一个, 删当前不崩 |
| `t5_hlist.c` | hlist (哈希链表) | pprev 指向前驱 next 字段地址, 删头不用特判; hlist_entry_safe 防 container_of(NULL) |
| `t6_zerooverhead.c` | 零开销反汇编 | -O2 下 container_of = 1 条 lea; 遍历循环 5 条指令无 call |
| `t7_kernel_patterns.c` | 内核真实用例 | 全局设备链表 + private_data + 按条件查找 |
| `t8_macro_expand.c` | 宏展开 | list_for_each_entry 完全展开为 for + container_of |
| `t9_rcu_list.c` | RCU 链表 | 写入顺序: 先 new->next, wmb, 最后 prev->next |
| `t10_pitfalls.c` | 边界与坑点 | 成员名写错零警告; 空链表遍历安全; POISON segfault |
| `t11_compare.c` | list vs slist vs array | 数组比链表快 568 倍 (cache 局部性) |

## 环境备注

- 编译器: gcc 13.3 (WSL Ubuntu 24.04), x86-64
- WSL 调用方式: `wsl -d Ubuntu --exec bash -c` (不用 `-lc`, 避免 profile 加载卡住)
- T10 需 `-Wno-array-bounds` (gcc -O2 会对 container_of 跨结构体越界误报)
- 输出含中文, 建议重定向到文件后查看 (避免 GBK/UTF-8 编码问题)
