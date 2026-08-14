# Bulk-update paths after Item folder migration
$ErrorActionPreference = "Stop"
$repo = Resolve-Path (Join-Path $PSScriptRoot "../..")
Set-Location $repo

$files = Get-ChildItem -Recurse -Include *.md,*.yml -File |
    Where-Object { $_.FullName -notmatch '\\target\\' -and $_.FullName -notmatch '\\\.git\\' }

foreach ($f in $files) {
    $c = [IO.File]::ReadAllText($f.FullName)
    $orig = $c

    # Item note paths: Item-NN-slug.md -> Item-NN-slug/README.md
    $c = [regex]::Replace($c, 'Item-(\d{2}-[a-z0-9-]+)\.md', 'Item-$1/README.md')

    # Old ER-demos -> co-located demo paths
    $c = $c -replace '\.\./ER-demos/item-03-option-result/', './demo/'
    $c = $c -replace '\.\./ER-demos/item-04-error-types/', './demo/'
    $c = $c -replace '\.\./ER-demos/item-04-core-error/', './demo-core-error/'
    $c = $c -replace '\.\./ER-demos/item-05-06-newtype/', '../Item-05-type-conversions/demo/'
    $c = $c -replace '\.\./ER-demos/item-15-borrow-checker/', './demo/'
    $c = $c -replace '\.\./ER-demos/item-16-miri/', './demo/'
    $c = $c -replace '\.\./ER-demos/item-18-dont-panic/', './demo/'
    $c = $c -replace '\.\./ER-demos/item-20-tlv/', './demo/'
    $c = $c -replace '\.\./ER-demos/item-22-visibility/', './demo/'
    $c = $c -replace '\.\./ER-demos/item-24-re-export/', './demo/'
    $c = $c -replace '\.\./ER-demos/item-26-feature-creep/', './demo/'
    $c = $c -replace '\.\./ER-demos/item-30-black-box/', './demo/'
    $c = $c -replace '\.\./ER-demos/item-33-no-std/', './demo/'
    $c = $c -replace '\.\./ER-demos/item-34-ffi-box/', './demo/'
    $c = $c -replace '\.\./ER-demos/item-35-bindgen/', './demo-bindgen/'
    $c = $c -replace '\.\./ER-demos/item-35-sys-workspace/', './demo-sys-workspace/'

    # Book / index: ER-demos paths
    $c = $c -replace '01-ER/ER-demos/item-05-06-newtype/', '01-ER/Chapter-01-Types/Item-05-type-conversions/demo/'
    $c = $c -replace '01-ER/ER-demos/item-03-option-result/', '01-ER/Chapter-01-Types/Item-03-option-result-transforms/demo/'
    $c = $c -replace '01-ER/ER-demos/item-04-error-types/', '01-ER/Chapter-01-Types/Item-04-idiomatic-error-types/demo/'
    $c = $c -replace '01-ER/ER-demos/item-04-core-error/', '01-ER/Chapter-01-Types/Item-04-idiomatic-error-types/demo-core-error/'
    $c = $c -replace '01-ER/ER-demos/item-15-borrow-checker/', '01-ER/Chapter-03-Concepts/Item-15-borrow-checker/demo/'
    $c = $c -replace '01-ER/ER-demos/item-16-miri/', '01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/demo/'
    $c = $c -replace '01-ER/ER-demos/item-18-dont-panic/', '01-ER/Chapter-03-Concepts/Item-18-dont-panic/demo/'
    $c = $c -replace '01-ER/ER-demos/item-20-tlv/', '01-ER/Chapter-03-Concepts/Item-20-avoid-over-optimize/demo/'
    $c = $c -replace '01-ER/ER-demos/item-22-visibility/', '01-ER/Chapter-04-Dependencies/Item-22-minimize-visibility/demo/'
    $c = $c -replace '01-ER/ER-demos/item-24-re-export/', '01-ER/Chapter-04-Dependencies/Item-24-re-export-api-types/demo/'
    $c = $c -replace '01-ER/ER-demos/item-26-feature-creep/', '01-ER/Chapter-04-Dependencies/Item-26-feature-creep/demo/'
    $c = $c -replace '01-ER/ER-demos/item-30-black-box/', '01-ER/Chapter-05-Tooling/Item-30-beyond-unit-tests/demo/'
    $c = $c -replace '01-ER/ER-demos/item-33-no-std/', '01-ER/Chapter-06-Beyond-Standard-Rust/Item-33-no-std/demo/'
    $c = $c -replace '01-ER/ER-demos/item-34-ffi-box/', '01-ER/Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/demo/'
    $c = $c -replace '01-ER/ER-demos/item-35-bindgen/', '01-ER/Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/demo-bindgen/'
    $c = $c -replace '01-ER/ER-demos/item-35-sys-workspace/', '01-ER/Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/demo-sys-workspace/'

    # Index-style ./ER-demos/item-* paths
    $c = $c -replace '\./ER-demos/item-03-option-result/', './Chapter-01-Types/Item-03-option-result-transforms/demo/'
    $c = $c -replace '\./ER-demos/item-04-error-types/', './Chapter-01-Types/Item-04-idiomatic-error-types/demo/'
    $c = $c -replace '\./ER-demos/item-04-core-error/', './Chapter-01-Types/Item-04-idiomatic-error-types/demo-core-error/'
    $c = $c -replace '\./ER-demos/item-05-06-newtype/', './Chapter-01-Types/Item-05-type-conversions/demo/'
    $c = $c -replace '\./ER-demos/item-15-borrow-checker/', './Chapter-03-Concepts/Item-15-borrow-checker/demo/'
    $c = $c -replace '\./ER-demos/item-16-miri/', './Chapter-03-Concepts/Item-16-avoid-unsafe/demo/'
    $c = $c -replace '\./ER-demos/item-18-dont-panic/', './Chapter-03-Concepts/Item-18-dont-panic/demo/'
    $c = $c -replace '\./ER-demos/item-20-tlv/', './Chapter-03-Concepts/Item-20-avoid-over-optimize/demo/'
    $c = $c -replace '\./ER-demos/item-22-visibility/', './Chapter-04-Dependencies/Item-22-minimize-visibility/demo/'
    $c = $c -replace '\./ER-demos/item-24-re-export/', './Chapter-04-Dependencies/Item-24-re-export-api-types/demo/'
    $c = $c -replace '\./ER-demos/item-26-feature-creep/', './Chapter-04-Dependencies/Item-26-feature-creep/demo/'
    $c = $c -replace '\./ER-demos/item-30-black-box/', './Chapter-05-Tooling/Item-30-beyond-unit-tests/demo/'
    $c = $c -replace '\./ER-demos/item-33-no-std/', './Chapter-06-Beyond-Standard-Rust/Item-33-no-std/demo/'
    $c = $c -replace '\./ER-demos/item-34-ffi-box/', './Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/demo/'
    $c = $c -replace '\./ER-demos/item-35-bindgen/', './Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/demo-bindgen/'
    $c = $c -replace '\./ER-demos/item-35-sys-workspace/', './Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/demo-sys-workspace/'

    # Item READMEs: ../ER-demos -> ../../ER-demos (one extra directory level)
    if ($f.FullName -match '\\Item-\d{2}-[^\\]+\\README\.md$') {
        $c = $c -replace '\.\./ER-demos/', '../../ER-demos/'
    }

    if ($c -ne $orig) {
        [IO.File]::WriteAllText($f.FullName, $c)
        Write-Host "updated $($f.FullName.Substring($repo.Path.Length + 1))"
    }
}

Write-Host "Link update done."
