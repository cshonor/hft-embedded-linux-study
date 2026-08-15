# 5. kubectl-trace（17.4）

> 底本：《BPF之巅》第 17 章，17.4 节（印刷 p752–755）

**kubectl-trace** 是一个 Kubernetes 命令行前端，用于在 Kubernetes 集群中的各个节点上运行 **bpftrace**。由 Lorenzo Fontana 创建，托管在 **IOVisor** 项目（原书链接 11）。

安装（需要已部署 Kubernetes）：

```bash
$ git clone https://github.com/iovisor/kubectl-trace.git
$ cd kubectl-trace
$ make
$ sudo cp ./output/bin/kubectl-trace /usr/local/bin
```

## 17.4.1 跟踪节点

kubectl 是 Kubernetes 命令行前端；kubectl-trace 支持在集群**节点**上运行 bpftrace。跟踪所有节点是最简单的选项，但要注意 **BPF 插桩的开销**——有大开销的 bpftrace 调用会影响整个集群节点。

完整的跟踪生命周期（run → get → logs → delete）：

```bash
$ kubectl trace run --node/ip-1-2-3-4 -f /usr/share/bpftrace/tools/vfsstat.bt
trace 8fc22ddb-5c84-11e9-9ad2-02d0df09784a created

$ kubectl trace get
NAME                                       NODE        STATUS    AGE
kubectl-trace-8fc22ddb-...                 ip-1-2-3-4  Running   3s

$ kubectl trace logs -f kubectl-trace-8fc22ddb-5c84-11e9-9ad2-02d0df09784a
00:02:54  @[vfs open]:  940
           @[vfs write]: 7015
           @[vfs read]:  7797
00:02:55  @[vfs write]:  252
           @[vfs open]:  289
           @[vfs read]:  924

$ kubectl trace delete kubectl-trace-8fc22ddb-5c84-11e9-9ad2-02d0df09784a
```

**关键认知**：该输出显示**整个节点**上所有 vfs 统计，不仅仅是 pod——因为 bpftrace 在主机上运行，kubectl-trace 也在**主机上下文**中，跟踪节点上的**所有应用程序**。对系统管理员有帮助，但许多用例需要聚焦容器内部进程。

## 17.4.2 跟踪 pod 和容器

bpftrace（kubectl-trace 基于它，继承相关限制）通过**匹配内核数据结构**间接支持容器。kubectl-trace 提供两种帮助：

1. 指定 **pod 名称**时，自动在正确的节点上定位并部署 bpftrace 程序
2. 引入额外变量 **`$container_pid`**——被赋值为容器 root 进程的 PID（**主机 PID 命名空间**），可用于只针对该 pod 的过滤或其他操作

> 注意：`$container_pid` 是容器内唯一进程的 PID。更复杂场景（容器内跑 init 或派生服务器）需修改工具把该 PID 映射到父/子 PID。

### 示例：只跟踪一个 pod 的 vfs 统计

创建指定 Docker 入口点的部署，确保 node 进程是容器内唯一进程：

```bash
$ cat <<'EOF' | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: node-hello
spec:
  selector:
    matchLabels:
      app: node-hello
  replicas: 1
  template:
    metadata:
      labels:
        app: node-hello
    spec:
      containers:
      - name: node-hello
        image: duluca/minimal-node-web-server
        command: ['node', "index"]
        ports:
        - containerPort: 3000
EOF
deployment.apps/node-hello created

$ kubectl get pods
NAME                               READY  STATUS   RESTARTS  AGE
node-hello-56b8dbc757-th2k2        1/1    Running  0         4s
```

复制 vfsstat.bt 为 vfsstat-pod.bt，加上按 `$container_pid` 过滤的谓词：

```bash
$ cat vfsstat-pod.bt
kprobe:vfs_read,
kprobe:vfs_write,
kprobe:vfs_fsync,
kprobe:vfs_open,
kprobe:vfs_create
/pid == $container_pid/
{ ... }

$ kubectl trace run pod/node-hello-56b8dbc757-th2k2 -f vfsstat-pod.bt
trace 552a2492-5c83-11e9-a598-02d0df09784a created

$ kubectl trace logs -f 552a2492-5c83-11e9-a598-02d0df09784a
Attaching 8 probes...
17:58:34  @[vfs open]:  1
           @[vfs read]:  3
           @[vfs write]:  4
17:58:36  @[vfs read]:  3
           @[vfs write]:  4
```

pod 级别的 vfs 操作比节点级别**少得多**——对大多空闲的 Web 服务器可以预见。

## 17.4.3 进一步阅读

更多信息见原书链接 12。

## HFT 关联

- K8s 上跑行情网关/策略 pod 时，kubectl-trace + `$container_pid` 过滤可远程临时插桩而不进入容器镜像——运维侧标准"手术刀"
- 记住教训：不带过滤的跟踪是**节点级**的（所有 pod 的数据混在一起），高开销脚本会拖累同节点邻居——交易类 pod 应独占节点（与第 15 章"吵闹邻居"呼应）

<details>
<summary>自测题</summary>

1. kubectl-trace 在节点上跑脚本时，默认跟踪范围是什么？为什么？
2. `$container_pid` 变量的值是什么命名空间下的什么 PID？有什么限制？
3. `kubectl trace run --node=... -f xxx.bt` 的完整生命周期有哪四条命令？

</details>
