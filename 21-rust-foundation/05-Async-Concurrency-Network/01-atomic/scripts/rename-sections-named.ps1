# Rename atomic section dirs: 1.3 -> 1.3-thread-spawn, demos full slug prefix
$ErrorActionPreference = "Stop"
$atomic = Resolve-Path (Join-Path $PSScriptRoot "..")

$ch1Map = @{
  "1.1" = @{ Slug = "1.1-process-and-threads"; Demos = @{} }
  "1.2" = @{ Slug = "1.2-borrowing-data-races"; Demos = @{} }
  "1.3" = @{
    Slug = "1.3-thread-spawn"
    Demos = @{
      "1.3-move-closure-demo.rs" = "1.3-thread-spawn-move-closure-demo.rs"
      "1.3-thread-example-demo.rs" = "1.3-thread-spawn-example-demo.rs"
      "1.3-spawn-join-demo.rs" = "1.3-thread-spawn-join-demo.rs"
      "1.3-spawn-return-demo.rs" = "1.3-thread-spawn-return-demo.rs"
      "1.3-usecall-demo.rs" = "1.3-thread-spawn-usecall-demo.rs"
    }
  }
  "1.4" = @{ Slug = "1.4-scoped-threads"; Demos = @{ "1.4-scoped-threads-demo.rs" = "1.4-scoped-threads-demo.rs" } }
  "1.5" = @{
    Slug = "1.5-shared-ownership"
    Demos = @{
      "1.5-static-demo.rs" = "1.5-shared-ownership-static-demo.rs"
      "1.5-box-leak-demo.rs" = "1.5-shared-ownership-box-leak-demo.rs"
      "1.5-arc-demo.rs" = "1.5-shared-ownership-arc-demo.rs"
    }
  }
  "1.6" = @{ Slug = "1.6-interior-mutability"; Demos = @{ "1.6-cell-refcell-demo.rs" = "1.6-interior-mutability-cell-refcell-demo.rs" } }
  "1.7" = @{ Slug = "1.7-send-sync"; Demos = @{ "1.7-send-sync-demo.rs" = "1.7-send-sync-demo.rs" } }
  "1.8" = @{
    Slug = "1.8-mutex-rwlock"
    Demos = @{
      "1.8-mutex-demo.rs" = "1.8-mutex-rwlock-mutex-demo.rs"
      "1.8-mutex-guard-lifetime-demo.rs" = "1.8-mutex-rwlock-guard-lifetime-demo.rs"
      "1.8-thread-advanced-demo.rs" = "1.8-mutex-rwlock-thread-advanced-demo.rs"
    }
  }
  "1.9" = @{ Slug = "1.9-parking-condvar"; Demos = @{ "1.9-condvar-demo.rs" = "1.9-parking-condvar-demo.rs" } }
  "1.10" = @{ Slug = "1.10-summary"; Demos = @{} }
}

function Move-RenameSection($chapterPath, $map) {
  foreach ($num in $map.Keys) {
    $slug = $map[$num].Slug
    $dest = Join-Path $chapterPath $slug
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    # sources: short num dir, long slug dir, or already correct
    @($num, $slug) | ForEach-Object {
      $src = Join-Path $chapterPath $_
      if ((Test-Path $src) -and ($src -ne $dest)) {
        Get-ChildItem $src -Filter "*.rs" -ErrorAction SilentlyContinue | ForEach-Object {
          Move-Item $_.FullName $dest -Force
        }
      }
    }
    foreach ($pair in $map[$num].Demos.GetEnumerator()) {
      $old = Join-Path $dest $pair.Key
      $new = Join-Path $dest $pair.Value
      if ((Test-Path $old) -and ($old -ne $new)) { Move-Item $old $new -Force }
    }
    if (-not (Get-ChildItem $dest -Filter "*.rs" -ErrorAction SilentlyContinue)) {
      Set-Content (Join-Path $dest ".gitkeep") "" -NoNewline
    }
    # remove empty short dirs
    $short = Join-Path $chapterPath $num
    if ((Test-Path $short) -and ($short -ne $dest)) { Remove-Item $short -Recurse -Force -ErrorAction SilentlyContinue }
  }
  # remove duplicate long dirs without matching slug key
  Get-ChildItem $chapterPath -Directory | Where-Object {
    $_.Name -match '^\d+\.\d+-' -and -not ($map.Values.Slug -contains $_.Name)
  } | ForEach-Object {
    $slugHit = $_.Name -replace '^(\d+\.\d+)-.*','$1'
    if ($map.ContainsKey($slugHit)) {
      $target = Join-Path $chapterPath $map[$slugHit].Slug
      Get-ChildItem $_.FullName -Filter "*.rs" | ForEach-Object { Move-Item $_.FullName $target -Force }
      Remove-Item $_.FullName -Recurse -Force
    }
  }
}

Move-RenameSection (Join-Path $atomic "Chapter-01-Rust-Concurrency-Basics") $ch1Map

$ch2Map = @{
  "2.1" = @{ Slug = "2.1-memory-model"; Demos = @{} }
  "2.2" = @{
    Slug = "2.2-load-store"
    Demos = @{
      "2.2-load-store-demo.rs" = "2.2-load-store-demo.rs"
      "2.2-lazy-init-demo.rs" = "2.2-load-store-lazy-init-demo.rs"
    }
  }
  "2.3" = @{ Slug = "2.3-fetch-modify"; Demos = @{ "2.3-fetch-modify-demo.rs" = "2.3-fetch-modify-demo.rs" } }
  "2.4" = @{ Slug = "2.4-cas"; Demos = @{ "2.4-id-allocator-demo.rs" = "2.4-cas-id-allocator-demo.rs" } }
  "2.5" = @{ Slug = "2.5-quick-demo"; Demos = @{ "2.5-quick-demo.rs" = "2.5-quick-demo.rs" } }
  "2.6" = @{ Slug = "2.6-fence"; Demos = @{ "2.6-fence-demo.rs" = "2.6-fence-demo.rs" } }
  "2.7" = @{ Slug = "2.7-seqcst"; Demos = @{ "2.7-seqcst-demo.rs" = "2.7-seqcst-demo.rs" } }
}
Move-RenameSection (Join-Path $atomic "Chapter-02-Atomics") $ch2Map

$ch4Map = @{ "4.1" = @{ Slug = "4.1-spin-lock"; Demos = @{ "4.1-spin-lock-demo.rs" = "4.1-spin-lock-demo.rs" } } }
Move-RenameSection (Join-Path $atomic "Chapter-04-Spin-Locks") $ch4Map

$ch5Map = @{ "5.1" = @{ Slug = "5.1-one-shot-channel"; Demos = @{ "5.1-one-shot-channel-demo.rs" = "5.1-one-shot-channel-demo.rs" } } }
Move-RenameSection (Join-Path $atomic "Chapter-05-Channels") $ch5Map

# ch03,06-10 slug dirs only
@(
  @{ Ch = "Chapter-03-Memory-Ordering"; Sections = @("3.1-why-ordering","3.2-five-orderings","3.3-happens-before","3.4-summary") },
  @{ Ch = "Chapter-06-Custom-Arc"; Sections = @("6.1-custom-arc") },
  @{ Ch = "Chapter-07-Processors"; Sections = @("7.1-cache-and-coherence","7.2-false-sharing") },
  @{ Ch = "Chapter-08-OS-Primitives"; Sections = @("8.1-futex","8.2-pthread-windows") },
  @{ Ch = "Chapter-09-Custom-Locks"; Sections = @("9.1-mutex-optimizations","9.2-write-priority-rwlock") },
  @{ Ch = "Chapter-10-Advanced-Concurrent-Data-Structures"; Sections = @("10.1-advanced-patterns","10.2-lock-free","10.3-design-practices","10.4-debug-interview") }
) | ForEach-Object {
  $cp = Join-Path $atomic $_.Ch
  foreach ($s in $_.Sections) {
    $d = Join-Path $cp $s
    New-Item -ItemType Directory -Force -Path $d | Out-Null
    if (-not (Get-ChildItem $d -Filter "*.rs" -ErrorAction SilentlyContinue)) {
      Set-Content (Join-Path $d ".gitkeep") "" -NoNewline
    }
    # remove bare 3.1 dirs
    $bare = $s -replace '^(\d+\.\d+)-.*','$1'
    $barePath = Join-Path $cp $bare
    if ((Test-Path $barePath) -and ($barePath -ne $d)) { Remove-Item $barePath -Recurse -Force -ErrorAction SilentlyContinue }
  }
}

Write-Host "atomic named sections done."
