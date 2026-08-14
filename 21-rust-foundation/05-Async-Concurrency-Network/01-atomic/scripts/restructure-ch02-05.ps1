# 重组 Chapter-02～05：笔记上提 + X.Y/ 仅 rs
$ErrorActionPreference = "Stop"
$atomic = Resolve-Path (Join-Path $PSScriptRoot "..")

function Ensure-Gitkeep($dir) {
  if (-not (Get-ChildItem $dir -Filter "*.rs" -ErrorAction SilentlyContinue)) {
    $gk = Join-Path $dir ".gitkeep"
    if (-not (Test-Path $gk)) { Set-Content $gk "" -NoNewline }
  }
}

# --- Chapter 02 ---
$ch2 = Join-Path $atomic "Chapter-02-Atomics"
$moves2 = @(
  @{ Sec = "2.2"; File = "use_atomic.rs"; New = "2.2-load-store-demo.rs" },
  @{ Sec = "2.2"; File = "lazy_init.rs"; New = "2.2-lazy-init-demo.rs" },
  @{ Sec = "2.3"; File = "use_atomic_operations.rs"; New = "2.3-fetch-modify-demo.rs" },
  @{ Sec = "2.4"; File = "id_allocator.rs"; New = "2.4-id-allocator-demo.rs" },
  @{ Sec = "2.5"; File = "quick_demo.rs"; New = "2.5-quick-demo.rs" },
  @{ Sec = "2.6"; File = "use_fence.rs"; New = "2.6-fence-demo.rs" },
  @{ Sec = "2.7"; File = "use_seqcst.rs"; New = "2.7-seqcst-demo.rs" }
)
foreach ($m in $moves2) {
  $d = Join-Path $ch2 $m.Sec
  New-Item -ItemType Directory -Force -Path $d | Out-Null
  $src = Join-Path $ch2 $m.File
  if (Test-Path $src) { Move-Item $src (Join-Path $d $m.New) -Force }
}
@("2.1") | ForEach-Object { New-Item -ItemType Directory -Force -Path (Join-Path $ch2 $_) | Out-Null; Ensure-Gitkeep (Join-Path $ch2 $_) }

# --- Chapter 04 ---
$ch4 = Join-Path $atomic "Chapter-04-Spin-Locks"
$d41 = Join-Path $ch4 "4.1"
New-Item -ItemType Directory -Force -Path $d41 | Out-Null
$sl = Join-Path $ch4 "spin_lock.rs"
if (Test-Path $sl) { Move-Item $sl (Join-Path $d41 "4.1-spin-lock-demo.rs") -Force }

# --- Chapter 05 ---
$ch5 = Join-Path $atomic "Chapter-05-Channels"
$d51 = Join-Path $ch5 "5.1"
New-Item -ItemType Directory -Force -Path $d51 | Out-Null
$os = Join-Path $ch5 "one_shot_channel.rs"
if (Test-Path $os) { Move-Item $os (Join-Path $d51 "5.1-one-shot-channel-demo.rs") -Force }

Write-Host "Chapters 02,04,05 code moved."
