# 云原生环境：对比 sidecar 模型

sidecar（如 Envoy 注入）的固有缺陷：
- 加 sidecar 要**重启 Pod**
- 依赖 YAML 注入流程，标注错了就漏掉 → 未被插桩
- Pod 内多容器 ready 顺序不可预测 → 启动变慢、竞态（Open Service Mesh 文档承认：Envoy ready 前应用流量全丢）
- 服务网格走 sidecar = 应用流量必须绕内核网络栈进代理容器，**加延迟**

eBPF 模型：内核插桩 → 节点全进程可见、动态加载、恶意程序逃不过（挖矿程序不会"贴心地"帮你注入 sidecar，但 eBPF 网络安全能监管主机所有流量）。
