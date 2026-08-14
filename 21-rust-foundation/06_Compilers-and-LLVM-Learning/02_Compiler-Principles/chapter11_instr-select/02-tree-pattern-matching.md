# 第 11 章 · 指令筛选 · §2 树模式匹配

← [本章目录](./README.md) · 上一节：[01-tree-walk.md](./01-tree-walk.md) · 下一节：[03-peephole-optimization.md](./03-peephole-optimization.md)

---

现代编译器的主流方案：**把 ISA 描述成树模式**，在 IR 树上找**最小代价覆盖（Tiling）**。

---

## 重写规则（Rewrite Rules）

目标指令集 = 一组**局部树模式** + 对应**机器指令模板**：

```text
模式:              生成:
  (ADD r1 r2)  →   add r1, r2
  (ADD (LOAD x) (LOAD y))  →   add (mem x), (mem y)   // 若 ISA 允许
  (MUL 2 (SHL x k))  →   lea x, [x*2^k]             // 融合示例
```

每条规则：**匹配** IR 子树形状 → **替换**为一条或几条目标指令。

---

## 寻找铺盖（Finding a Tiling）

| 概念 | 说明 |
|------|------|
| **Tiling / 铺盖** | 用模式**不重叠、完整地覆盖**整棵 IR 树 |
| **代价模型** | 每条规则有 cost（延迟、字节数、寄存器压力…） |
| **优化目标** | 所有合法铺盖中 **总代价最小**（或近优） |

```text
     IR 树                    两种铺盖（示意）
       +                    A: 3 条独立指令
      / \                   B: 1 条融合 load-add
    load  c                 → 选 cost 更低者
     |
     a
```

**算法**：动态规划 / 自底向上标注每节点**最优子铺盖代价** — 类似 ch8 DAG 最优覆盖，但在**机器模式**层。

---

## 自动化工具

与 ch2 **正则 → 扫描器生成器** 类比：

| 前端 | 后端 |
|------|------|
| 正则描述 token | **树模式**描述指令 |
| lex/flex 生成扫描器 | **BURG / IBURG / Twig** 生成匹配器 |
| | **LLVM TableGen** 描述 `Pat<(add …), …>` |

**工程收益**：

- 新增一条 ISA 扩展 → 加规则，**再生匹配器**
- 减少手写 if-else 巨型 switch

**LLVM**：`*.td` 里 `def : Pat<…>`；`llvm-tblgen` 产出 `SelectionDAG` 匹配代码。

---

## 与 ch5 / ch8

| 章节 | 相似点 |
|------|--------|
| ch5 **DAG** | IR 也是树/ DAG 形态 |
| ch8 **值编号 / DAG 覆盖** | 消冗余；本章 DAG 覆盖为**选指令** |

---

## 自测

- [ ] 重写规则 vs 树 walk 一对一映射
- [ ] Tiling 为何是优化问题而非语法问题
- [ ] TableGen 在 LLVM 里扮演什么角色
