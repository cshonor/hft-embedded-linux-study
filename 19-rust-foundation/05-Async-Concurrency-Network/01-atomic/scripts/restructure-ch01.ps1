# 一次性重组 Chapter-01：笔记上提、文件夹改为纯编号 1.1/1.2…
$ErrorActionPreference = "Stop"
$ch = Join-Path $PSScriptRoot "..\Chapter-01-Rust-Concurrency-Basics"
Set-Location $ch

$sections = @(
  @{ Old = "1.1-process-and-threads"; Num = "1.1"; Md = "1.1-process-and-threads.md" },
  @{ Old = "1.2-borrowing-data-races"; Num = "1.2"; Md = "1.2-borrowing-data-races.md" },
  @{ Old = "1.3-thread-spawn"; Num = "1.3"; Md = "1.3-thread-spawn.md" },
  @{ Old = "1.4-scoped-threads"; Num = "1.4"; Md = "1.4-scoped-threads.md" },
  @{ Old = "1.5-shared-ownership"; Num = "1.5"; Md = "1.5-shared-ownership.md" },
  @{ Old = "1.6-interior-mutability"; Num = "1.6"; Md = "1.6-interior-mutability.md" },
  @{ Old = "1.7-send-sync"; Num = "1.7"; Md = "1.7-send-sync.md" },
  @{ Old = "1.8-mutex-rwlock"; Num = "1.8"; Md = "1.8-mutex-rwlock.md" },
  @{ Old = "1.9-parking-condvar"; Num = "1.9"; Md = "1.9-parking-condvar.md" },
  @{ Old = "1.10-summary"; Num = "1.10"; Md = "1.10-summary.md" }
)

foreach ($s in $sections) {
  $oldDir = Join-Path $ch $s.Old
  $newDir = Join-Path $ch $s.Num
  if (Test-Path $oldDir) {
    New-Item -ItemType Directory -Force -Path $newDir | Out-Null
    Get-ChildItem $oldDir -Filter "*.rs" | ForEach-Object { Move-Item $_.FullName $newDir -Force }
    $notes = Join-Path $oldDir "notes.md"
    if (Test-Path $notes) { Move-Item $notes (Join-Path $ch $s.Md) -Force }
    Remove-Item $oldDir -Recurse -Force
  } elseif (-not (Test-Path $newDir)) {
    New-Item -ItemType Directory -Force -Path $newDir | Out-Null
  }
  if (-not (Get-ChildItem $newDir -Filter "*.rs" -ErrorAction SilentlyContinue)) {
    $gk = Join-Path $newDir ".gitkeep"
    if (-not (Test-Path $gk)) { "" | Out-File $gk -Encoding ascii -NoNewline }
  }
}

# 删除章根重复的旧 flat .rs
$legacy = @(
  "move_closure.rs","thread_example.rs","usejoin.rs","userecall.rs","usecall.rs",
  "usescope.rs","usestatic.rs","useboxleak.rs","use_rc_arc.rs","use_cell_refcell.rs",
  "use_send_sync.rs","use_mutex.rs","use_mutex_guard_lifetime.rs","thread_advanced.rs","use_condvar.rs"
)
foreach ($f in $legacy) {
  $p = Join-Path $ch $f
  if (Test-Path $p) { Remove-Item $p -Force }
}
Write-Host "Chapter-01 restructured."
