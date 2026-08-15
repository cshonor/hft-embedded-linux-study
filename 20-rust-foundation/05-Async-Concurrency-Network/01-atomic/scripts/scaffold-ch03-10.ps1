# 为 Chapter-03～10 建立 X.Y-slug.md 骨架（无 demo 时不建空目录）
$ErrorActionPreference = "Stop"
$atomic = Resolve-Path (Join-Path $PSScriptRoot "..")

function Add-Section($chapterPath, $sections) {
  foreach ($s in $sections) {
    if ($s.HasDemo) {
      New-Item -ItemType Directory -Force -Path (Join-Path $chapterPath $s.Dir) | Out-Null
    }
    $md = Join-Path $chapterPath $s.Md
    if (-not (Test-Path $md)) {
      $demoLine = if ($s.HasDemo) {
        "> Demo：[$($s.Dir)/](./$($s.Dir)/)"
      } else {
        "> **本节无独立 Demo**（待补或见别章交叉引用）。"
      }
      @"
# $($s.Title)

$demoLine

## 要点

（从原 ``本章学习笔记.md`` 拆分精读；此处为小节占位。）

## 扩展

- [ ] 精读待写

"@ | Set-Content $md -Encoding UTF8
    }
  }
}

Add-Section (Join-Path $atomic "Chapter-03-Memory-Ordering") @(
  @{ Dir="3.1-why-ordering"; Md="3.1-why-ordering.md"; Title="3.1 为什么需要内存序"; HasDemo=$false },
  @{ Dir="3.2-five-orderings"; Md="3.2-five-orderings.md"; Title="3.2 五种 Ordering"; HasDemo=$false },
  @{ Dir="3.3-happens-before"; Md="3.3-happens-before.md"; Title="3.3 Happens-Before"; HasDemo=$false },
  @{ Dir="3.4-summary"; Md="3.4-summary.md"; Title="3.4 小结与背诵"; HasDemo=$false }
)

Add-Section (Join-Path $atomic "Chapter-04-Spin-Locks") @(
  @{ Dir="4.1-spin-lock"; Md="4.1-spin-lock.md"; Title="4.1 自旋锁与 Acquire/Release"; HasDemo=$true }
)

Add-Section (Join-Path $atomic "Chapter-05-Channels") @(
  @{ Dir="5.1-one-shot-channel"; Md="5.1-one-shot-channel.md"; Title="5.1 One-Shot Channel"; HasDemo=$true }
)

Add-Section (Join-Path $atomic "Chapter-06-Custom-Arc") @(
  @{ Dir="6.1-custom-arc"; Md="6.1-custom-arc.md"; Title="6.1 手写 Arc"; HasDemo=$false }
)

Add-Section (Join-Path $atomic "Chapter-07-Processors") @(
  @{ Dir="7.1-cache-and-coherence"; Md="7.1-cache-and-coherence.md"; Title="7.1 缓存与一致性"; HasDemo=$false },
  @{ Dir="7.2-false-sharing"; Md="7.2-false-sharing.md"; Title="7.2 伪共享"; HasDemo=$false }
)

Add-Section (Join-Path $atomic "Chapter-08-OS-Primitives") @(
  @{ Dir="8.1-futex"; Md="8.1-futex.md"; Title="8.1 Futex"; HasDemo=$false },
  @{ Dir="8.2-pthread-windows"; Md="8.2-pthread-windows.md"; Title="8.2 pthread / Windows"; HasDemo=$false }
)

Add-Section (Join-Path $atomic "Chapter-09-Custom-Locks") @(
  @{ Dir="9.1-mutex-optimizations"; Md="9.1-mutex-optimizations.md"; Title="9.1 Mutex 优化"; HasDemo=$false },
  @{ Dir="9.2-write-priority-rwlock"; Md="9.2-write-priority-rwlock.md"; Title="9.2 写优先 RwLock"; HasDemo=$false }
)

Add-Section (Join-Path $atomic "Chapter-10-Advanced-Concurrent-Data-Structures") @(
  @{ Dir="10.1-advanced-patterns"; Md="10.1-advanced-patterns.md"; Title="10.1 进阶并发模式"; HasDemo=$false },
  @{ Dir="10.2-lock-free"; Md="10.2-lock-free.md"; Title="10.2 无锁基础"; HasDemo=$false },
  @{ Dir="10.3-design-practices"; Md="10.3-design-practices.md"; Title="10.3 设计思路"; HasDemo=$false },
  @{ Dir="10.4-debug-interview"; Md="10.4-debug-interview.md"; Title="10.4 排错与面试"; HasDemo=$false }
)

Write-Host "Chapters 03-10 skeleton created (notes only unless HasDemo)."
