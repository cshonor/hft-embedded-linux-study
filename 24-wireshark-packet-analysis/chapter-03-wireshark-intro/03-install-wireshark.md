# 3.3 安装 Wireshark

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 逐步安装：[../cheatsheet/install-and-verify.md](../cheatsheet/install-and-verify.md)

**核心主旨**：Windows / Linux / macOS 安装流程；**Npcap** 为 Windows 抓包必备。

## 核心知识点

### 3.3.1 Windows（Win10/11 64 位）

| 步 | 操作 |
|----|------|
| 1 | [wireshark.org/download](https://www.wireshark.org/download.html) → **Windows 64-bit Installer**（`.exe`） |
| 2 | 许可协议：同意 |
| 3 | **Choose Components**：Wireshark、TShark 默认勾选即可（Npcap 不在此页） |
| 4 | **Packet Capture**：**务必勾选 Install Npcap**（必选） |
| 5 | Npcap 子向导：许可 **I Agree**，其余一路默认 |
| 6 | 安装路径：默认；等待安装结束 |
| 7 | 开始菜单打开 **Wireshark** |

带截图的逐步说明：[install-and-verify.md § Windows](../cheatsheet/install-and-verify.md#windowswin1011-64-位)

| 组件 | 作用 |
|------|------|
| **Npcap** | 在 **Packet Capture** 页安装；无则无法抓本机实时流量。本机若仍为 **WinPcap**，安装时会提示卸载并由 Npcap 接替 |
| **TShark** | 与 Wireshark 同装，命令行抓包 |
| **USBPcap** | USB 抓包，以太网排障可不装 |

**PATH（可选）**：将 `C:\Program Files\Wireshark` 加入环境变量 → 命令行可用 **tshark**。

---

### 3.3.2 Linux（Ubuntu / Debian）

```bash
sudo apt update
sudo apt install wireshark -y
sudo usermod -aG wireshark $USER   # 免每次 sudo
# 注销重新登录
```

RHEL/Fedora：`sudo dnf install wireshark wireshark-cli`

---

### 3.3.3 macOS

| 步 | 操作 |
|----|------|
| 1 | 官网下载 **.dmg** |
| 2 | 拖入 **Applications** |
| 3 | 若被拦截：**系统设置** → **隐私与安全性** → **仍要打开** |
| 4 | 允许安装 **tshark** 等命令行组件 |

`brew install --cask wireshark`

## 抓包/实操记录

完整验证步骤（含首次抓包命令）：[install-and-verify.md](../cheatsheet/install-and-verify.md)

| 检查项 | 通过标准 |
|--------|----------|
| 驱动 | `Capture` → `Interfaces` 网卡非灰 |
| 试抓 | Start → 上网 → 包列表递增 |
| 命令行 | `tshark -D` 有接口列表 |

## 疑问与总结

- **装得上 ≠ 抓得到**：无 Npcap、无权限、虚拟网卡限制 → Interfaces 空或 0 包。
- Windows 无原生 **tcpdump**，用 **tshark** 或 WSL（见第 6 章）。
