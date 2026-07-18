# 第 8 章 数组

**Arrays**

## 本章讲什么

**一维回顾、二维行优先、传参退化、指针数组 vs 数组指针、字符数组、VLA、初始化与陷阱**。DPDK `pkts[]`、内核二维表、HFT 批量缓冲的核心容器。

## 学习重点

- `arr[i] ≡ *(arr+i)`；传参 **`+ len`**
- 二维：**`*(*(buf+i)+j)`**；传参 **`int (*)[COLS]`**
- **`int *a[5]`** vs **`int (*b)[5]`**
- 字符串指针数组 vs **`char buf[N][M]`**
- **VLA** 栈风险；**`{0}`** 清零
- 栈 mega 数组 → **malloc**

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | `pkts[BURST]`、port×queue 二维 |
| 内核 | 向量表、设备二维映射 |
| HFT | 连续行情池、常量 opcode 表 |

## 实操（建议完成）

1. 模拟 `pkts[32]`  
2. `int (*row)[3]` 遍历  
3. 字符串指针数组 vs 二维 char  
4. 大 VLA 栈溢出  
5. 二维传参仅首维省略  
6. sizeof 对比三种类型  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch06 指针；ch07 栈 |
| 后序 | ch09 字符串；ch11 堆数组；ch13 |
| 配套 | 《C陷阱与缺陷》ch03、ch07 |

## 小节

- [8.1 一维数组](./8.1-one-dimensional-arrays/8.1-one-dimensional-arrays.md)（8.1.1–8.1.11）
- [8.2 多维数组](./8.2-multidimensional-arrays/8.2-multidimensional-arrays.md)（8.2.1–8.2.7）
- [8.3 指针数组](./8.3-指针数组.md)
