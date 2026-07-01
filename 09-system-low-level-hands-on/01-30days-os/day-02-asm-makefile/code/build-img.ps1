# 把 ipl.bin（512 B）拼成 helloos.img（1.44 MB 软盘）
# 用法：.\build-img.ps1
# 前置：同目录已有 ipl.bin（nasm -f bin helloos.asm -o ipl.bin）

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$ipl = Join-Path $here 'ipl.bin'
$img = Join-Path $here 'helloos.img'
$size = 1474560  # 2880 × 512

if (-not (Test-Path $ipl)) {
    Write-Error "Missing ipl.bin — run: nasm -f bin helloos.asm -o ipl.bin"
}

$iplBytes = [System.IO.File]::ReadAllBytes($ipl)
if ($iplBytes.Length -ne 512) {
    Write-Error "ipl.bin must be 512 bytes, got $($iplBytes.Length)"
}
if ($iplBytes[0x1FE] -ne 0x55 -or $iplBytes[0x1FF] -ne 0xAA) {
    Write-Error "ipl.bin missing boot signature 55 AA at offset 0x1FE"
}

$disk = New-Object byte[] $size
[Array]::Copy($iplBytes, 0, $disk, 0, 512)
[System.IO.File]::WriteAllBytes($img, $disk)

Write-Host "OK: helloos.img ($size bytes) — ipl.bin embedded at offset 0"
Write-Host "Run: qemu-system-i386 -fda helloos.img -boot a"
