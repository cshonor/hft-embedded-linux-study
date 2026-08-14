# One-time: Item-NN-slug.md -> Item-NN-slug/README.md; demos -> Item-*/demo*
$ErrorActionPreference = "Stop"
$erRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $erRoot

# 1) Move notes into per-Item folders
Get-ChildItem -Path "Chapter-*" -Directory | ForEach-Object {
    $chapter = $_.FullName
    Get-ChildItem -Path $chapter -Filter "Item-*.md" -File | ForEach-Object {
        $dirName = $_.BaseName
        $targetDir = Join-Path $chapter $dirName
        if (-not (Test-Path $targetDir)) { New-Item -ItemType Directory -Path $targetDir | Out-Null }
        $dest = Join-Path $targetDir "README.md"
        if (Test-Path $dest) { throw "Already exists: $dest" }
        git mv $_.FullName $dest
        Write-Host "note -> $targetDir/README.md"
    }
}

# 2) Demo relocations (source under ER-demos -> Item folder)
$demoMoves = [ordered]@{
    "ER-demos/item-03-option-result" = "Chapter-01-Types/Item-03-option-result-transforms/demo"
    "ER-demos/item-04-error-types" = "Chapter-01-Types/Item-04-idiomatic-error-types/demo"
    "ER-demos/item-04-core-error" = "Chapter-01-Types/Item-04-idiomatic-error-types/demo-core-error"
    "ER-demos/item-05-06-newtype" = "Chapter-01-Types/Item-05-type-conversions/demo"
    "ER-demos/item-15-borrow-checker" = "Chapter-03-Concepts/Item-15-borrow-checker/demo"
    "ER-demos/item-16-miri" = "Chapter-03-Concepts/Item-16-avoid-unsafe/demo"
    "ER-demos/item-18-dont-panic" = "Chapter-03-Concepts/Item-18-dont-panic/demo"
    "ER-demos/item-20-tlv" = "Chapter-03-Concepts/Item-20-avoid-over-optimize/demo"
    "ER-demos/item-22-visibility" = "Chapter-04-Dependencies/Item-22-minimize-visibility/demo"
    "ER-demos/item-24-re-export" = "Chapter-04-Dependencies/Item-24-re-export-api-types/demo"
    "ER-demos/item-26-feature-creep" = "Chapter-04-Dependencies/Item-26-feature-creep/demo"
    "ER-demos/item-30-black-box" = "Chapter-05-Tooling/Item-30-beyond-unit-tests/demo"
    "ER-demos/item-33-no-std" = "Chapter-06-Beyond-Standard-Rust/Item-33-no-std/demo"
    "ER-demos/item-34-ffi-box" = "Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/demo"
    "ER-demos/item-35-bindgen" = "Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/demo-bindgen"
    "ER-demos/item-35-sys-workspace" = "Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/demo-sys-workspace"
}

foreach ($entry in $demoMoves.GetEnumerator()) {
    $src = $entry.Key
    $dest = $entry.Value
    if (-not (Test-Path $src)) { Write-Host "skip missing $src"; continue }
    $parent = Split-Path $dest -Parent
    if (-not (Test-Path $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
        Write-Host "mkdir $parent"
    }
    git mv $src $dest
    Write-Host "demo -> $dest"
}

Write-Host "Done. Update Cargo.toml members and links."
