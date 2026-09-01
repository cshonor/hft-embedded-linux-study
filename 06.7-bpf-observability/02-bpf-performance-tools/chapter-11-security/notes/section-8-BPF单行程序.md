# 8. BPF 单行程序（11.3）

> 底本：《BPF之巅》第 11 章 安全，11.3 节（印刷 p542–543）

## 8.1 三个安全单行（BCC 与 bpftrace 对照）

```bash
# 1. 统计 PID 1234 的安全审计事件数（LSM 钩子）
funccount -p 1234 'security_*'
bpftrace -e 'kprobe:security_* /pid == 1234/ { @[probe] = count(); }'

# 2. 跟踪 PAM（可插入身份验证模块）会话开始
trace 'pam:pam_start "%s: %s", arg1, arg2'
bpftrace -e 'u:/lib/x86_64-linux-gnu/libpam.so.0:pam_start \
  { printf("%s: %s\n", str(arg0), str(arg1)); }'

# 3. 跟踪内核模块加载
trace 't:module:module_load "load: %s", args->name'
bpftrace -e 't:module:module_load { printf("load: %s\n", str(args->name)); }'
```

## 8.2 示例输出解读

**funccount security_***（LSM 钩子事件计数）：

```
security_task_setpgid        （低频）
security_task_alloc
security_inode_alloc
security_prepare_creds
security_file_permission     13
security_vm_enough_memory_mm 27
security_file_ioctl          34
```

- 每个 LSM 钩子（263 个函数匹配 security_*）都可单独进一步跟踪——安全审计的全量入口清单。

**PAM 会话开始**：

```
PID    COMM  FUNC       ...
25568  sshd  pam_start  sshd:bgregg
25641  sudo  pam_start  sudo:bgregg
```

- 显示 sshd/sudo 为用户开启 PAM 会话；跟踪其他 pam_* 函数可看完整认证链。

## 8.3 方法论

- `security_*`（LSM）是安全分析的**稳定入口面**（对照 11.1.4 策略第 2 步）。
- libpam 的 uprobe 展示用户态库插桩：认证栈（login/sshd/sudo → PAM）全链路可见。

## HFT 关联

- 管理机上 funccount security_* 可快速回答"这台机器正在发生多少安全检查"，异常升高提示扫描/提权活动。
- PAM 跟踪 + setuids + bashreadline 组成登录链完整审计（谁、何时、以何权限、执行了什么）。

<details>
<summary>自测题</summary>

1. security_* 函数族是什么？为什么是稳定的插桩入口？
2. pam_start 的两个参数各是什么？

<details><summary>参考答案</summary>

1. LSM（Linux Security Module）框架的钩子函数族，内核把每个安全敏感操作（setuid、文件权限、模块加载…）都汇聚到对应 security_* 函数裁决。稳定是因为它是 LSM 的**导出 API**（给 SELinux/AppArmor 等用的接口），改名等于改 ABI；同时天然按安全语义分类——"安全相关的全量入口清单"。
2. arg0 = service 名（如 "sshd"、"sudo"，哪个 PAM 服务在启动会话）；arg1 = user 名。bpftrace 单行里 printf 的 `str(arg0), str(arg1)` 即输出 `sshd:bgregg` 这样的"服务:用户"对。
</details>
</details>
