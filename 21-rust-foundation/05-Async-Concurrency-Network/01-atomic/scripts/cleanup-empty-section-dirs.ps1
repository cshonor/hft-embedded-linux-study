# 删除无 .rs 的小节目录；删除小节内的 notes.md / mod.rs（违反规范）
$ErrorActionPreference = "Stop"
$atomic = Resolve-Path (Join-Path $PSScriptRoot "..")
$asyncRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\async_tokio")

function Remove-EmptySectionDirs($root) {
  Get-ChildItem $root -Recurse -Directory | ForEach-Object {
    $dir = $_
    if ($dir.Name -notmatch '^\d+\.\d+(-|$)') { return }
    $rs = @(Get-ChildItem $dir.FullName -Filter "*.rs" -ErrorAction SilentlyContinue)
    if ($rs.Count -eq 0) {
      Remove-Item $dir.FullName -Recurse -Force
      Write-Host "Removed empty: $($dir.FullName)"
    }
  }
}

function Remove-IllegalSubsectionFiles($root) {
  Get-ChildItem $root -Recurse -File | Where-Object {
    $_.Name -in @("notes.md", "mod.rs") -and $_.DirectoryName -match '\\\d+\.\d+-'
  } | ForEach-Object {
    Remove-Item $_.FullName -Force
    Write-Host "Removed illegal: $($_.FullName)"
  }
}

Remove-EmptySectionDirs $atomic
Remove-IllegalSubsectionFiles $atomic
Remove-EmptySectionDirs $asyncRoot
Remove-IllegalSubsectionFiles $asyncRoot

Write-Host "Done."
