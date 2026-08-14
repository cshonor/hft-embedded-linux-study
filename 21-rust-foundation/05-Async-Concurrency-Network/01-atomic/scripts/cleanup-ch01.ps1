# 清理 Chapter-01：合并重复目录、删除小节内 mod.rs
$ErrorActionPreference = "Stop"
$ch = Resolve-Path (Join-Path $PSScriptRoot "..\Chapter-01-Rust-Concurrency-Basics")

$map = @{
  "1.1-process-and-threads" = "1.1"
  "1.2-borrowing-data-races" = "1.2"
  "1.3-thread-spawn" = "1.3"
  "1.4-scoped-threads" = "1.4"
  "1.5-shared-ownership" = "1.5"
  "1.6-interior-mutability" = "1.6"
  "1.7-send-sync" = "1.7"
  "1.8-mutex-rwlock" = "1.8"
  "1.9-parking-condvar" = "1.9"
  "1.10-summary" = "1.10"
}

foreach ($old in $map.Keys) {
  $oldPath = Join-Path $ch $old
  $newPath = Join-Path $ch $map[$old]
  if (Test-Path $oldPath) {
    New-Item -ItemType Directory -Force -Path $newPath | Out-Null
    Get-ChildItem $oldPath -Filter "*.rs" -Recurse | ForEach-Object {
      Move-Item $_.FullName $newPath -Force
    }
    Remove-Item $oldPath -Recurse -Force
  }
}

Get-ChildItem $ch -Directory | Where-Object { $_.Name -match '^\d+\.\d+$' } | ForEach-Object {
  Get-ChildItem $_.FullName -Filter "mod.rs" | Remove-Item -Force
  if (-not (Get-ChildItem $_.FullName -Filter "*.rs")) {
    $gk = Join-Path $_.FullName ".gitkeep"
    if (-not (Test-Path $gk)) { Set-Content $gk "" -NoNewline }
  }
}

Write-Host "Chapter-01 cleanup done."
