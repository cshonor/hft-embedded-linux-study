# Split ER Item README.md -> hub + topics/*.md (skip Item-01, already custom)
$ErrorActionPreference = "Stop"
$erRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

$topicMap = @(
    @{ Pattern = '^##\s*1\.'; File = '01-core-concepts.md'; Title = '核心知识点' }
    @{ Pattern = '^##\s*2\.'; File = '02-logic-flow.md'; Title = '逻辑脉络' }
    @{ Pattern = '^##\s*3\.'; File = '03-key-takeaways.md'; Title = '重点结论' }
    @{ Pattern = '^##\s*4\.'; File = '04-examples.md'; Title = '案例与代码' }
    @{ Pattern = '^##\s*5\.'; File = '05-pitfalls.md'; Title = '易错细节' }
    @{ Pattern = '^##\s*记忆'; File = 'cheat-sheet.md'; Title = '记忆卡片' }
)

function Fix-TopicLinks([string]$body) {
    $b = $body
    # one level deeper than Item README
    $b = $b -replace '\(\.\./\.\./00-Book/', '(../../../../00-Book/'
    $b = $b -replace '\(\.\./\.\./ER-demos/', '(../../../ER-demos/'
    $b = $b -replace '\(\.\./ER-拓展', '(../../../ER-拓展'
    $b = $b -replace '\(\.\./Chapter-', '(../../Chapter-'
    $b = $b -replace '\(\.\./Item-', '(../../Item-'
    $b = $b -replace '\(\.\./\.\./Chapter-', '(../../Chapter-'
    return $b
}

function Fix-HubLinks([string]$body) {
    $b = $body
    $b = $b -replace '\(\.\./ER-本书目录', '(../../ER-本书目录'
    $b = $b -replace '\(\.\./ER-拓展', '(../../ER-拓展'
    $b = $b -replace '\(\.\./\.\./00-Book/', '(../../../00-Book/'
    $b = $b -replace '\(\.\./\.\./ER-demos/', '(../../ER-demos/'
    $b = $b -replace '\(\.\./Chapter-', '(../../Chapter-'
    $b = $b -replace '\(\.\./Item-', '(../Item-'
    return $b
}

Get-ChildItem -Path $erRoot -Recurse -Filter README.md |
    Where-Object { $_.FullName -match '\\Item-\d{2}-[^\\]+\\README\.md$' } |
    ForEach-Object {
        $readmePath = $_.FullName
        $itemDir = $_.DirectoryName
        $itemName = Split-Path $itemDir -Leaf

        if ($itemName -eq 'Item-01-express-data-structures') {
            Write-Host "skip $itemName (custom split)"
            return
        }

        $raw = [IO.File]::ReadAllText($readmePath)
        if ($raw -match '## 专项笔记') {
            Write-Host "skip $itemName (already hub)"
            return
        }

        $lines = $raw -split "`r?`n"
        $sections = @{}
        $currentKey = '_header'
        $currentLines = [System.Collections.Generic.List[string]]::new()

        foreach ($line in $lines) {
            $matched = $false
            foreach ($tm in $topicMap) {
                if ($line -match $tm.Pattern) {
                    if ($currentLines.Count -gt 0) {
                        $sections[$currentKey] = ($currentLines -join "`n").TrimEnd()
                    }
                    $currentKey = $tm.File
                    $currentLines = [System.Collections.Generic.List[string]]::new()
                    $matched = $true
                    break
                }
            }
            if ($line -match '^##\s*6\.') {
                if ($currentLines.Count -gt 0) {
                    $sections[$currentKey] = ($currentLines -join "`n").TrimEnd()
                }
                $currentKey = '06-extension'
                $currentLines = [System.Collections.Generic.List[string]]::new()
                $matched = $true
            }
            if (-not $matched -and $currentKey -eq '_header' -and $line -match '^##\s*1\.') {
                # handled above
            }
            $currentLines.Add($line) | Out-Null
        }
        if ($currentLines.Count -gt 0) {
            $sections[$currentKey] = ($currentLines -join "`n").TrimEnd()
        }

        if (-not $sections.ContainsKey('01-core-concepts.md')) {
            Write-Host "WARN: no section 1 in $itemName"
            return
        }

        # Item number from folder
        if ($itemName -match 'Item-(\d{2})') { $num = [int]$Matches[1] } else { $num = 0 }

        $topicsDir = Join-Path $itemDir 'topics'
        New-Item -ItemType Directory -Path $topicsDir -Force | Out-Null

        $tableRows = @()
        foreach ($tm in $topicMap) {
            if (-not $sections.ContainsKey($tm.File)) { continue }
            $body = $sections[$tm.File]
            # strip leading ## section title line
            $bodyLines = $body -split "`r?`n"
            if ($bodyLines[0] -match '^## ') {
                $body = ($bodyLines[1..($bodyLines.Length - 1)] -join "`n").Trim()
            }
            $body = Fix-TopicLinks $body
            $topicTitle = if ($tm.File -eq 'cheat-sheet.md') { '背诵提纲' } else { $tm.Title }
            $topicContent = @"
# Item $num · $($tm.Title)

← [Item $num 目录](../README.md)

$body
"@
            $outPath = Join-Path $topicsDir $tm.File
            [IO.File]::WriteAllText($outPath, $topicContent.TrimEnd() + "`n")
            $label = $tm.File -replace '\.md$','' -replace '^0','' -replace '^cheat-sheet','—'
            if ($tm.File -eq 'cheat-sheet.md') {
                $tableRows += "| — | 背诵提纲 | [cheat-sheet.md](./cheat-sheet.md) |"
            } else {
                $n = $tm.File.Substring(0, 2)
                $tableRows += "| $n | $($tm.Title) | [$($tm.File)](./$($tm.File)) |"
            }
        }

        # Build hub from header (through Book table)
        $header = $sections['_header']
        # trim anything after section 1 starts in header (shouldn't happen)
        $headerLines = $header -split "`r?`n"
        $hubHeader = [System.Collections.Generic.List[string]]::new()
        foreach ($hl in $headerLines) {
            if ($hl -match '^##\s*1\.') { break }
            $hubHeader.Add($hl) | Out-Null
        }
        $hubHeaderText = Fix-HubLinks ($hubHeader -join "`n")

        # one-liner from section 3
        $oneLiner = '见 [03-key-takeaways.md](./03-key-takeaways.md)。'
        if ($sections.ContainsKey('03-key-takeaways.md')) {
            $t3 = $sections['03-key-takeaways.md']
            if ($t3 -match '(?m)^1\.\s+\*\*(.+?)\*\*') {
                $oneLiner = "**$($Matches[1])**"
            } elseif ($t3 -match '(?m)^-\s+\*\*(.+?)\*\*') {
                $oneLiner = $Matches[1]
            }
        }

        # logic excerpt from section 2
        $logicBlock = ''
        if ($sections.ContainsKey('02-logic-flow.md')) {
            $t2 = $sections['02-logic-flow.md']
            if ($t2 -match '(?s)```text\r?\n(.*?)```') {
                $logicBlock = @"

---

## 逻辑脉络

```text
$($Matches[1].Trim())
```

"@
            }
        }

        $extBlock = ''
        if ($sections.ContainsKey('06-extension')) {
            $ext = $sections['06-extension']
            $ext = Fix-HubLinks $ext
            $extBlock = "`n---`n`n$ext"
        } else {
            $extBlock = @"

---

## 后续拓展

> [ER-拓展索引 § Item $($num.ToString('00'))](../../ER-拓展索引.md#item-$($num.ToString('00')))
"@
        }

        $table = ($tableRows -join "`n")
        $hub = @"
$hubHeaderText

---

## 一句话

$oneLiner

---

## 专项笔记（按需点开）

| # | 专题 | 阅读 |
|---|------|------|
$table
$logicBlock
$extBlock
"@

        [IO.File]::WriteAllText($readmePath, $hub.TrimEnd() + "`n")
        Write-Host "split $itemName"
    }

Write-Host "Done."
