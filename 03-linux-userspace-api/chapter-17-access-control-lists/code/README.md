# Ch17 `code/` 目录说明

TLPI 第 17 章（Access Control Lists）的代码。

| 类别 | 文件 | 实测状态 |
|------|------|---------|
| 判定算法复现 | `c17_1_acl_algorithm.c` | ✅ 本机实测（macOS/clang 23.1.0；纯逻辑，全平台可跑） |
| 习题 17-1 | `ex17_1_listacls.c` | ⛔ Linux 专有（libacl，`-lacl`），Pi5/ext4 复测 |
| 原书镜像 | `acl_view.c`(Listing 17-1) `acl_update.c`(补充) | ⛔ Linux 专有（libacl） |
| 支撑 | `tlpi_hdr.h` 替身 | — |

```bash
# 全平台：判定算法实测（本机已跑通，输出钉进 17.2/17.4）
cc -Wall -Wextra -o c17_1_acl_algorithm c17_1_acl_algorithm.c && ./c17_1_acl_algorithm

# Linux（Pi5，需 libacl1-dev）：真机 ACL
gcc -Wall -o ex17_1_listacls ex17_1_listacls.c -lacl && ./ex17_1_listacls u liming /tmp/data
gcc -Wall -o acl_view acl_view.c -lacl && ./acl_view /tmp/data
gcc -Wall -o acl_update acl_update.c -lacl
```

> ⚠️ POSIX draft ACL（`system.posix_acl_*` xattr + libacl）是 Linux 生态；macOS 无对应 API/常量。
> 本机的"实测"仅覆盖判定算法的纯逻辑复现（9 用例 + mask 清零实验），与 TLPI §17.2 算法逐条一致。
