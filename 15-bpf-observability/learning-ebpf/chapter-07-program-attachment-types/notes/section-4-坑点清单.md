# 坑点清单

1. **一个接口只能挂一个 XDP 程序**——再挂报 `Device or resource busy`；多逻辑要用程序内部分发（第 2 章 PROG_ARRAY 尾调用）
2. kprobe 挂到被内联的函数 = 静默没有入口点
3. kprobe 声明参数时只能省略**尾部**参数，不能跳过中间的
4. tracepoint 前 4 个公共字段（common_type/flags/preempt_count/pid）访问即验证失败
5. uprobe 的库路径是架构全路径，跨架构部署要分别定义；容器内路径 ≠ 宿主机路径
6. Go < 1.17 二进制参数在栈上，pt_regs 方案失效
7. kfunc 无稳定性承诺，生产分发慎用（helper 才有 UAPI 保证）
