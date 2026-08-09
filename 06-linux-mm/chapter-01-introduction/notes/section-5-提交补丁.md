# Ch 1 §5 提交补丁 (Submitting Patches)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

### 5. 提交补丁 (Submitting Patches)

把改动合入 **主线 (mainline)** 的要点：

| 必须 | 说明 |
|------|------|
| **`Documentation/CodingStyle`** | 缩进、命名、注释风格 |
| **`Documentation/SubmittingPatches`** | 补丁格式、changelog、`Signed-off-by` |
| **发对邮件列表** | **尽早、频繁** 发到 **子系统列表**（VM 相关补丁 → **linux-mm** 等），常 **Cc LKML** 引发讨论 |
| **维护者层级** | 子系统 **maintainer / lieutenant** 审查 → **推荐给 Linus** 合入 |

**文化：** 内核是 **邮件列表驱动** 的；补丁是 **对话的单位**，不是只扔一个 PR 就结束（现代部分子系统也接受 **GitLab/GitHub** 镜像，但 mm 传统仍以列表为主）。

---

## 本章带走的三句话

1. **工具：** `diff`/`patch`（或 git）+ 在线交叉引用（Elixir/LXR）+ 必要时 call graph。  
2. **读 `mm/`：** 别从 arch init 硬啃；**OOM → vmalloc → page_alloc → VMA** 由简入深。  
3. **改内核：** 先 `CodingStyle` + `SubmittingPatches`，补丁发到 **对的 mm 列表**。

---
