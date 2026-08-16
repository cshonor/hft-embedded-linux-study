# 5.10 bpftrace 的控制流

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.10 节（印刷 p163–164）

## 内容详解

bpftrace 支持 3 种测试：**过滤器、三元运算符、if 语句**。布尔运算符：`==`、`!=`、`>`、`<`、`>=`、`<=`、`&&`、`||`；表达式可用括号分组。

**循环支持有限**：BPF 验证器出于安全拒绝可能无限循环的代码；bpftrace 支持**循环展开**，未来支持有界循环。

### 5.10.1 过滤器

```
probe/filter/{ action }
```

`/pid == 123/` 只有 pid 等于 123 才执行动作（内核态过滤，便宜）。

### 5.10.2 三元操作符

```
test ? true_statement : false_statement
```

经典用例——求绝对值：`$abs = $x > 0 ? $x : -$x;`

### 5.10.3 if 语句

```
if (test) { true_statements }
if (test) { true_statements } else { false_statements }
```

用例：IPv4/IPv6 分支处理 `if ($inetfamily == AF_INET) {...} else {...}`。**不支持 else if**（可嵌套 if 代替）。

### 5.10.4 循环展开

```
unroll(count) { statements }
```

- count 是**编译期常量**，最大 **20**；不能传变量——循环次数必须在 BPF 编译阶段确定；
- 背景：BPF 运行在受限环境，验证器必须能证明程序可终止；
- Linux 5.3 起内核支持 **BPF 有界循环**，bpftrace 后续版本在 unroll() 之外提供 for/while。

## HFT 关联

- 内核态过滤（`/.../`）永远优先于用户态过滤——把条件写进探针行，事件不过滤就不出内核；
- 需要遍历数组/链表的场景在老版本上只能 unroll(20) 硬展开，新内核+新版 bpftrace 才有真正的 for/while——写脚本前先确认版本支持。

## 陷阱

- ⚠️ 无 else if；无 while(true)；普通 for/while 在 5.3 内核之前不可用（unroll 上限 20 且必须常量）。
- ⚠️ 三元/if 里的复杂表达式同样受验证器约束（除法、指针访问规则），编译报错时先简化表达式。

<details>
<summary>自测题</summary>

1. unroll(count) 的两个限制？
   <details><summary>答案</summary>count 必须是编译期常量；最大值 20。</details>

2. 为什么 BPF 限制循环？
   <details><summary>答案</summary>验证器必须证明程序能终止、不会死循环挂起内核。</details>
</details>
