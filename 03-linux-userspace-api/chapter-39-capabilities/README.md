# TLPI 第 39 章 — Capabilities

**优先级**：🔴（最小特权、容器、替代 SUID）  
**前置**：[Ch9 凭证](../chapter-09-process-credentials/notes.md) · [Ch38 特权安全](../chapter-38-secure-privileged/notes.md) · [Ch16 xattr](../chapter-16-extended-attributes/notes.md)  
**后置**：[Ch40 登录记账](../chapter-40-login-accounting/notes.md)

---

## 小节目录

- [39.1 动机](./notes/39.1-section-39-1.md)
- [39.3 进程能力集（**每线程**一份）](./notes/39.3-process-thread-capabilities.md)
- [39.5 exec 转换（简化）](./notes/39.5-exec.md)
- [39.6 UID 与能力](./notes/39.6-uid.md)
- [39.7 API](./notes/39.7-api.md)

---

## 章节目标


动机；进程 5 集 + 文件能力；exec 转换；Bounding/Ambient；UID 切换影响；libcap / `setcap`；capability-aware vs dumb。

---


---

## 易错清单


1. 能力是**线程**粒度  
2. 从 Permitted 删掉难自愈  
3. Ambient 不服务 root exec 传递  
4. 需 FS xattr 支持文件能力  
5. Bounding 只减  
6. 新项目：文件能力 > SUID-root  

---


---

## 实验清单


1. `setcap`/`getcap` 对比 SUID  
2. libcap 临时 Effective  
3. exec 后 `/proc/.../status`  
4. （选）Bounding / Ambient  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | Effective 才真正授权 |
| 2 | Permitted = 上限；Bounding = exec 天花板 |
| 3 | 文件能力存在 xattr |
| 4 | File Effective 是 1 bit |
| 5 | 替 SUID-root 用文件能力 |
| 6 | 按需抬 Eff，用完清掉 |

---


---

## 参考


- Kerrisk · TLPI Ch39  
- `man 7 capabilities` · `man 3 libcap` · `man 8 setcap`


---

## 代码示例

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/capability.h>
#include <string.h>

/* Ch39 Linux 能力 (Capabilities) — 细粒度权限替代全 root。
 * 演示 capget/capset 获取和设置能力。
 * 编译: gcc -o ch39_demo ch39_demo.c -lcap
 * 需要安装: apt install libcap-dev */

int main(void) {
    /* 获取当前进程的能力 */
    struct __user_cap_header_struct hdr = {
        .version = _LINUX_CAPABILITY_VERSION_3,
        .pid = 0  /* 0 = 当前进程 */
    };
    struct __user_cap_data_struct data[2];

    if (capget(&hdr, data) < 0) {
        perror("capget (need libcap)");
        return 1;
    }

    /* 打印有效能力集 */
    printf("Effective capabilities: 0x%x 0x%x\n", data[0].effective, data[1].effective);
    printf("Permitted capabilities: 0x%x 0x%x\n", data[0].permitted, data[1].permitted);
    printf("Inheritable capabilities: 0x%x 0x%x\n", data[0].inheritable, data[1].inheritable);

    /* 检查特定能力 */
    if (data[0].effective & (1 << CAP_NET_BIND_SERVICE))
        printf("Has CAP_NET_BIND_SERVICE (can bind < 1024)\n");
    else
        printf("No CAP_NET_BIND_SERVICE\n");

    if (data[0].effective & (1 << CAP_SYS_ADMIN))
        printf("Has CAP_SYS_ADMIN (very powerful)\n");
    else
        printf("No CAP_SYS_ADMIN\n");

    /* 文件能力: getcap/setcap 命令行工具 */
    printf("\nUse 'getcap <binary>' to check file capabilities\n");
    printf("Use 'setcap cap_net_bind_service=ep <binary>' to grant\n");
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
