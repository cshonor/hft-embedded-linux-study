# MikanOS · 全书目录（第 0–31 章 + 附录）

> **ゼロからの OS 自作入門** · 内田公太 · [官方目次](http://zero.osdev.jp/toc.html)

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 与 CPU 模式 / 分页 / 中断 / 调度强相关 |
| 🟡 | GUI/FS 实现；加深工程结构 |
| ⚪ | 应用层功能；时间紧可后补 |

## 第 0–6 章 · UEFI 启动与输入

| 章 | 主题 | 目录 slug（规划） | 标签 |
|----|------|-------------------|------|
| 0 | OS って個人で作れるの？ | `chapter-00-intro` | 🟡 |
| 1 | PC の仕組みとハローワールド | `chapter-01-hello-uefi` | 🔴 |
| 2 | EDK II 入門とメモリマップ | `chapter-02-edk2-memmap` | 🔴 |
| 3 | 画面表示とブートローダ | `chapter-03-bootloader` | 🟡 |
| 4 | ピクセル描画と make | `chapter-04-pixel-make` | ⚪ |
| 5 | 文字表示とコンソール | `chapter-05-console` | ⚪ |
| 6 | マウス入力と PCI | `chapter-06-mouse-pci` | 🟡 |

## 第 7–14 章 · 中断 · 内存 · 多任务

| 章 | 主题 | 目录 slug | 标签 |
|----|------|-----------|------|
| 7 | 割り込みと FIFO | `chapter-07-interrupt-fifo` | 🔴 |
| 8 | メモリ管理 | `chapter-08-memory` | 🔴 |
| 9 | 重ね合わせ処理 | `chapter-09-layers` | ⚪ |
| 10 | ウィンドウ | `chapter-10-window` | ⚪ |
| 11 | タイマと ACPI | `chapter-11-timer-acpi` | 🔴 |
| 12 | キー入力 | `chapter-12-keyboard` | ⚪ |
| 13 | マルチタスク (1) | `chapter-13-multitask1` | 🔴 |
| 14 | マルチタスク (2) | `chapter-14-multitask2` | 🔴 |

## 第 15–20 章 · Shell · FS · 分页 ·  syscall

| 章 | 主题 | 目录 slug | 标签 |
|----|------|-----------|------|
| 15 | ターミナル | `chapter-15-terminal` | ⚪ |
| 16 | コマンド | `chapter-16-commands` | ⚪ |
| 17 | ファイルシステム | `chapter-17-filesystem` | 🟡 |
| 18 | アプリケーション | `chapter-18-apps` | ⚪ |
| 19 | **ページング** | `chapter-19-paging` | 🔴 |
| 20 | **システムコール** | `chapter-20-syscall` | 🔴 |

## 第 21–31 章 · 应用与 IPC

| 章 | 主题 | 目录 slug | 标签 |
|----|------|-----------|------|
| 21–28 | GUI · 文件读写 · 日文 · 重定向 | `chapter-21` … `chapter-28` | ⚪ |
| 29 | **アプリ間通信** | `chapter-29-ipc` | 🟡 |
| 30–31 | おまけ · これからの道 | `chapter-30-bonus` · `chapter-31-next` | ⚪ |

## 附录

| 附录 | 内容 |
|------|------|
| A | 開発環境インストール → [SETUP.md](./SETUP.md) |
| B | MikanOS の入手 |
| C | EDK II ファイル説明 |
| D | C++ テンプレート |
| E | iPXE |
| F | ASCII 表 |

---

**笔记文件：** 随学习进度在 `chapter-XX-slug/notes/` 下增补；尚未创建的章仅保留上表 slug 规划。
