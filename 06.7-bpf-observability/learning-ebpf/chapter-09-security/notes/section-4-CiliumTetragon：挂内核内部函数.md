# Cilium Tetragon：挂内核内部函数

LSM 普及前的另一条路：把 eBPF 程序挂到**任意内核函数**（不限于稳定接口）。

- 依据：syscall/LSM 只是 3000 万行内核代码中极小的稳定部分；大量内部函数事实上多年未变；新内核普及需数年，不兼容有充足时间修复
- Tetragon 贡献者含内核开发者，凭内部知识挑出**安全且信息完备**的挂点
- K8s 自定义资源 **TracingPolicy** 声明式定义：挂点 + 条件 + 动作

```yaml
spec:
  kprobes:
  - call: "fd_install"        # 文件打开后在 fd 数组装入 file 指针（此时文件结构已填充完）
    matchArgs:
    - index: 1
      operator: "Prefix"
      values: ["/etc/"]       # 只关心 /etc/ 下的文件
```

- 挂 `fd_install` 而非 `open` syscall 的原因：它在**文件数据结构填充完之后**调用——和 LSM 同理，天然免疫 TOCTOU
- 内核内过滤：只把**超出策略**的事件报告用户态，而不是全量上报再筛
