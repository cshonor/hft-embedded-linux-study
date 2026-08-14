# Book-aligned section index .md (does not touch 本章学习笔记.md)
$ErrorActionPreference = "Stop"
$atomic = Resolve-Path (Join-Path $PSScriptRoot "..")

function Write-IndexMd($path, $heading, $bookSec, $demoPath, $extra) {
  if ($demoPath -and $demoPath.Length -gt 0) {
    $demoLine = "> **Demo**：[${demoPath}](./${demoPath})"
  } else {
    $demoLine = "> **Demo**: (pending)"
  }
  $lines = @(
    "# $heading",
    "",
    "> **完整正文**：[本章学习笔记.md](./本章学习笔记.md)（实体书 §$bookSec）",
    "> **书目**：[全书目录-与实体书一致.md](../全书目录-与实体书一致.md)",
    "> **对照**：[章节与小节对照表.md](../章节与小节对照表.md)"
  )
  if ($extra -and $extra.Length -gt 0) { $lines += $extra }
  $lines += $demoLine
  $lines += ""
  $utf8 = New-Object System.Text.UTF8Encoding $true
  [IO.File]::WriteAllText($path, ($lines -join "`r`n"), $utf8)
  Write-Host "OK $($path | Split-Path -Leaf)"
}

function Scaffold-Chapter($dir, $sections) {
  $chPath = Join-Path $atomic $dir
  Write-Host $dir
  foreach ($s in $sections) {
    $row = @($s)
    $slug = $row[0]
    $md = Join-Path $chPath ($slug + ".md")
    Write-IndexMd $md $row[1] $row[2] $row[3] $row[4]
  }
}

# Each row: slug, heading, bookSec, demoDir, extra (use "" not $null)
Scaffold-Chapter "Chapter-01-Rust-Concurrency-Basics" @(
  , @("1.1-threads-in-rust", "1.1 Threads in Rust", "1.1", "1.3-thread-spawn", "")
  , @("1.2-scoped-threads", "1.2 Scoped Threads", "1.2", "1.4-scoped-threads", "")
  , @("1.3-shared-ownership", "1.3 Shared Ownership and Reference Counting", "1.3", "1.5-shared-ownership", "> Sub: Statics, Leaking, Rc/Arc")
  , @("1.4-borrowing-data-races", "1.4 Borrowing and Data Races", "1.4", "", "")
  , @("1.5-interior-mutability", "1.5 Interior Mutability", "1.5", "1.6-interior-mutability", "> Sub: Cell, RefCell, Mutex/RwLock, Atomics, UnsafeCell")
  , @("1.6-send-sync", "1.6 Thread Safety: Send and Sync", "1.6", "1.7-send-sync", "")
  , @("1.7-mutex-rwlock", "1.7 Locking: Mutexes and RwLocks", "1.7", "1.8-mutex-rwlock", "> Sub: Rust Mutex, Lock Poisoning, RwLock")
  , @("1.8-parking-condvar", "1.8 Waiting: Parking and Condition Variables", "1.8", "1.9-parking-condvar", "> Sub: Thread Parking, Condition Variables")
  , @("1.9-summary", "1.9 Summary", "1.9", "", "")
)

Scaffold-Chapter "Chapter-02-Atomics" @(
  , @("2.1-atomic-load-store", "2.1 Atomic Load and Store Operations", "2.1", "2.2-load-store", "")
  , @("2.2-fetch-and-modify", "2.2 Fetch-and-Modify Operations", "2.2", "2.3-fetch-modify", "")
  , @("2.3-compare-and-exchange", "2.3 Compare-and-Exchange Operations", "2.3", "2.4-cas", "")
  , @("2.4-summary", "2.4 Summary", "2.4", "2.5-quick-demo", "")
)

Scaffold-Chapter "Chapter-03-Memory-Ordering" @(
  , @("3.1-reordering-and-optimizations", "3.1 Reordering and Optimizations", "3.1", "", "> Sub: Compiler/Hardware Reordering")
  , @("3.2-the-memory-model", "3.2 The Memory Model", "3.2", "../Chapter-02-Atomics/2.2-load-store", "> Sub: Happens-Before, Spawning/Joining")
  , @("3.3-memory-order-options", "3.3 Memory Order Options", "3.3", "../Chapter-02-Atomics/2.7-seqcst", "> Sub: Relaxed, Release/Acquire, Consume, SeqCst")
  , @("3.4-fences", "3.4 Fences", "3.4", "../Chapter-02-Atomics/2.6-fence", "")
  , @("3.5-common-misconceptions", "3.5 Common Misconceptions", "3.5", "", "")
  , @("3.6-summary", "3.6 Summary", "3.6", "", "")
)

Scaffold-Chapter "Chapter-04-Spin-Locks" @(
  , @("4.1-minimal-implementation", "4.1 A Minimal Implementation", "4.1", "4.1-spin-lock", "")
  , @("4.2-unsafe-spin-lock", "4.2 An Unsafe Spin Lock", "4.2", "4.1-spin-lock", "")
  , @("4.3-safe-lock-guard", "4.3 A Safe Interface Using a Lock Guard", "4.3", "4.1-spin-lock", "")
  , @("4.4-summary", "4.4 Summary", "4.4", "", "")
)

Scaffold-Chapter "Chapter-05-Channels" @(
  , @("5.1-mutex-based-channel", "5.1 A Simple Mutex-Based Channel", "5.1", "", "")
  , @("5.2-unsafe-one-shot-channel", "5.2 An Unsafe One-Shot Channel", "5.2", "5.1-one-shot-channel", "")
  , @("5.3-runtime-checks-safety", "5.3 Safety Through Runtime Checks", "5.3", "", "")
  , @("5.4-types-safety", "5.4 Safety Through Types", "5.4", "", "")
  , @("5.5-borrowing-avoid-allocation", "5.5 Borrowing to Avoid Allocation", "5.5", "", "")
  , @("5.6-blocking", "5.6 Blocking", "5.6", "", "")
  , @("5.7-summary", "5.7 Summary", "5.7", "", "")
)

Scaffold-Chapter "Chapter-06-Custom-Arc" @(
  , @("6.1-basic-reference-counting", "6.1 Basic Reference Counting", "6.1", "6.1-custom-arc", "")
  , @("6.2-testing-it", "6.2 Testing It", "6.2", "6.1-custom-arc", "")
  , @("6.3-mutation", "6.3 Mutation", "6.3", "", "")
  , @("6.4-weak-pointers", "6.4 Weak Pointers", "6.4", "", "")
  , @("6.5-testing-weak", "6.5 Testing It (Weak)", "6.5", "", "")
  , @("6.6-optimizing", "6.6 Optimizing", "6.6", "", "")
  , @("6.7-summary", "6.7 Summary", "6.7", "", "")
)

Scaffold-Chapter "Chapter-07-Processors" @(
  , @("7.1-processor-instructions", "7.1 Processor Instructions", "7.1", "", "> Sub: Load/Store, RMW, x86/ARM atomics")
  , @("7.2-caching", "7.2 Caching", "7.2", "", "> Sub: Coherence, MESI, Performance")
  , @("7.3-reordering", "7.3 Reordering", "7.3", "", "> Sub: x86-64, ARM64, Experiment")
  , @("7.4-memory-fences", "7.4 Memory Fences", "7.4", "../Chapter-02-Atomics/2.6-fence", "")
  , @("7.5-summary", "7.5 Summary", "7.5", "", "")
)

Scaffold-Chapter "Chapter-08-OS-Primitives" @(
  , @("8.1-futex", "8.1 Futex", "8.1", "", "> Sub: Waiting, Waking, Shared Futexes")
  , @("8.2-thread-parks", "8.2 Thread Parks", "8.2", "../Chapter-01-Rust-Concurrency-Basics/1.9-parking-condvar", "")
  , @("8.3-condition-variables", "8.3 Condition Variables", "8.3", "../Chapter-01-Rust-Concurrency-Basics/1.9-parking-condvar", "")
  , @("8.4-summary", "8.4 Summary", "8.4", "", "")
)

Scaffold-Chapter "Chapter-09-Custom-Locks" @(
  , @("9.1-mutex", "9.1 Mutex", "9.1", "", "> Sub: Avoiding Syscalls, Optimizing, Benchmarks")
  , @("9.2-condition-variable", "9.2 Condition Variable", "9.2", "", "> Sub: Avoiding Syscalls, Spurious Wakeup")
  , @("9.3-reader-writer-lock", "9.3 Reader-Writer Lock", "9.3", "", "> Sub: Writer Busy Loop, Writer Starvation")
  , @("9.4-summary", "9.4 Summary", "9.4", "", "")
)

Scaffold-Chapter "Chapter-10-Advanced-Concurrent-Data-Structures" @(
  , @("10.1-semaphores", "10.1 Semaphores", "10.1", "", "")
  , @("10.2-rcu", "10.2 RCU", "10.2", "", "")
  , @("10.3-lock-free-linked-list", "10.3 Lock-Free Linked List", "10.3", "", "")
  , @("10.4-queue-based-locks", "10.4 Queue-Based Locks", "10.4", "", "")
  , @("10.5-blocking-locks", "10.5 Blocking Locks", "10.5", "", "")
  , @("10.6-seqlocks", "10.6 Seqlocks", "10.6", "", "")
  , @("10.7-teaching-materials", "10.7 Teaching Materials", "10.7", "", "")
)

$app = Join-Path $atomic "Appendix"
New-Item -ItemType Directory -Force -Path $app | Out-Null
Write-IndexMd (Join-Path $app "A-rust-memory-model.md") "Appendix A – The Rust Memory Model" "A" "" "> 见 [Atomics与内存序-贯通笔记.md](../Atomics与内存序-贯通笔记.md)"
Write-IndexMd (Join-Path $app "B-bibliography.md") "Appendix B – Bibliography" "B" "" ""
Write-IndexMd (Join-Path $app "README.md") "Appendix（附录）" "" "" "> [A-rust-memory-model.md](./A-rust-memory-model.md) · [B-bibliography.md](./B-bibliography.md)"

Write-Host "Done."
