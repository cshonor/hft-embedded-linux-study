$er = Join-Path $PSScriptRoot ".."
Get-ChildItem -Path $er -Recurse -Filter README.md | Where-Object { $_.FullName -match '\\Item-\d{2}-' } | ForEach-Object {
    $c = [IO.File]::ReadAllText($_.FullName)
    $n = $c -replace '\]\(\./Item-', '](../Item-'
    if ($n -ne $c) {
        [IO.File]::WriteAllText($_.FullName, $n)
        Write-Host "fixed $($_.FullName)"
    }
}
