# Ch1 · Windows 原生 · 编译 BOOTX64.EFI
# 需已安装 LLVM（https://releases.llvm.org/）并勾选 Add LLVM to system PATH
# 用法（PowerShell，在 code\ 目录）：
#   .\build.ps1
#   .\build.ps1 -Run    # 需已装 QEMU + OVMF（见 SETUP.md）

param(
    [switch]$Run
)

$ErrorActionPreference = "Stop"

# 验证 clang
clang --version | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Error "clang 不在 PATH。请安装 LLVM 并勾选 'Add LLVM to system PATH'，重开终端后再试。"
}

Write-Host "==> clang --target=x86_64-elf -ffreestanding -c hello.c"
clang --target=x86_64-elf -ffreestanding -c hello.c -o hello.o
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$espBoot = Join-Path "esp" "EFI\BOOT"
New-Item -ItemType Directory -Force -Path $espBoot | Out-Null
$efiOut = Join-Path $espBoot "BOOTX64.EFI"

Write-Host "==> lld-link -> $efiOut"
lld-link /subsystem:efi_application /entry:EfiMain hello.o "/out:$efiOut"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "OK: $efiOut"

if ($Run) {
    $qemu = Get-Command qemu-system-x86_64 -ErrorAction SilentlyContinue
    if (-not $qemu) {
        Write-Error "未找到 qemu-system-x86_64。请安装 QEMU for Windows 并加入 PATH。"
    }
    # 常见 OVMF 路径（按本机安装调整）
    $ovmfCandidates = @(
        "$env:ProgramFiles\qemu\share\edk2\x64\OVMF_CODE.fd",
        "$env:ProgramFiles\qemu\share\OVMF.fd",
        "C:\Program Files\qemu\share\edk2-x64\OVMF_CODE.fd"
    )
    $ovmf = $ovmfCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $ovmf) {
        Write-Warning "未找到 OVMF_CODE.fd，请手动指定 -bios 路径。跳过 QEMU。"
        exit 0
    }
    $espFull = (Resolve-Path "esp").Path
    Write-Host "==> QEMU + OVMF"
    & qemu-system-x86_64 -bios $ovmf -drive "format=raw,file=fat:rw:$espFull" -m 512M
}
