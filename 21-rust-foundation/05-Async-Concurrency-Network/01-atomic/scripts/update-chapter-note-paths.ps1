# 更新 本章学习笔记.md 中的旧 .rs 路径 → X.Y-slug/ 新路径
$ErrorActionPreference = "Stop"
$atomic = Resolve-Path (Join-Path $PSScriptRoot "..")

$replacements = @{
  "atomic/Chapter-01-Rust-Concurrency-Basics/本章学习笔记.md" = @(
    @("配套代码见本目录 ``*.rs``", "配套代码见 ``1.Y-slug/`` 子目录（``mod.rs`` 挂接）；小节索引见章根 ``1.Y-slug.md``"),
    @("``use_condvar.rs``", "``1.9-parking-condvar/1.9-parking-condvar-demo.rs``"),
    @("``usejoin.rs``", "``1.3-thread-spawn/1.3-thread-spawn-join-demo.rs``"),
    @("``thread_example.rs``", "``1.3-thread-spawn/1.3-thread-spawn-example-demo.rs``"),
    @("``usescope.rs``", "``1.4-scoped-threads/1.4-scoped-threads-demo.rs``"),
    @("``use_rc_arc.rs``", "``1.5-shared-ownership/1.5-shared-ownership-arc-demo.rs``"),
    @("``use_mutex.rs``", "``1.8-mutex-rwlock/1.8-mutex-rwlock-mutex-demo.rs``"),
    @("``thread_advanced.rs``", "``1.8-mutex-rwlock/1.8-mutex-rwlock-thread-advanced-demo.rs``")
  )
  "atomic/Chapter-02-Atomics/本章学习笔记.md" = @(
    @("``use_atomic.rs``", "``2.2-load-store/2.2-load-store-demo.rs``"),
    @("``lazy_init.rs``", "``2.2-load-store/2.2-load-store-lazy-init-demo.rs``"),
    @("``use_atomic_operations.rs``", "``2.3-fetch-modify/2.3-fetch-modify-demo.rs``"),
    @("``id_allocator.rs``", "``2.4-cas/2.4-cas-id-allocator-demo.rs``"),
    @("``quick_demo.rs``", "``2.5-quick-demo/2.5-quick-demo.rs``"),
    @("``use_fence.rs``", "``2.6-fence/2.6-fence-demo.rs``"),
    @("``use_seqcst.rs``", "``2.7-seqcst/2.7-seqcst-demo.rs``")
  )
  "atomic/Chapter-03-Memory-Ordering/本章学习笔记.md" = @(
    @("Chapter-02-Atomics/use_atomic.rs", "../Chapter-02-Atomics/2.2-load-store/2.2-load-store-demo.rs"),
    @("``use_atomic.rs``", "``../Chapter-02-Atomics/2.2-load-store/2.2-load-store-demo.rs``"),
    @("``use_seqcst.rs``", "``../Chapter-02-Atomics/2.7-seqcst/2.7-seqcst-demo.rs``"),
    @("``use_fence.rs``", "``../Chapter-02-Atomics/2.6-fence/2.6-fence-demo.rs``"),
    @("``id_allocator.rs``", "``../Chapter-02-Atomics/2.4-cas/2.4-cas-id-allocator-demo.rs``"),
    @("[spin_lock.rs](../Chapter-04-Spin-Locks/spin_lock.rs)", "[4.1-spin-lock/4.1-spin-lock-demo.rs](../Chapter-04-Spin-Locks/4.1-spin-lock/4.1-spin-lock-demo.rs)"),
    @("Chapter-04-Spin-Locks/spin_lock.rs", "../Chapter-04-Spin-Locks/4.1-spin-lock/4.1-spin-lock-demo.rs")
  )
  "atomic/Chapter-04-Spin-Locks/本章学习笔记.md" = @(
    @("``spin_lock.rs``", "``4.1-spin-lock/4.1-spin-lock-demo.rs``")
  )
  "atomic/Chapter-05-Channels/本章学习笔记.md" = @(
    @("``one_shot_channel.rs``", "``5.1-one-shot-channel/5.1-one-shot-channel-demo.rs``"),
    @("``id_allocator.rs``", "``../Chapter-02-Atomics/2.4-cas/2.4-cas-id-allocator-demo.rs``"),
    @("``spin_lock.rs``", "``../Chapter-04-Spin-Locks/4.1-spin-lock/4.1-spin-lock-demo.rs``")
  )
  "atomic/Chapter-10-Advanced-Concurrent-Data-Structures/本章学习笔记.md" = @(
    @("``use_condvar.rs``", "``../Chapter-01-Rust-Concurrency-Basics/1.9-parking-condvar/1.9-parking-condvar-demo.rs``")
  )
}

foreach ($rel in $replacements.Keys) {
  $path = Join-Path (Split-Path $atomic -Parent) $rel
  if (-not (Test-Path $path)) { Write-Warning "Skip missing $path"; continue }
  $text = Get-Content $path -Raw -Encoding UTF8
  foreach ($pair in $replacements[$rel]) {
    $text = $text.Replace($pair[0], $pair[1])
  }
  Set-Content $path $text -Encoding UTF8 -NoNewline
  Write-Host "Updated $rel"
}

Write-Host "Done."
