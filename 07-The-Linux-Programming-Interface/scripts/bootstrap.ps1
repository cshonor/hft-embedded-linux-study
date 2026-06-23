# One-shot scaffold for TLPI/ chapter folders (run from repo root: .\TLPI\scripts\bootstrap.ps1)
$ErrorActionPreference = "Stop"
$tlpi = Split-Path $PSScriptRoot -Parent
if (-not (Test-Path $tlpi)) { New-Item -ItemType Directory -Path $tlpi | Out-Null }

$templatePath = Join-Path $PSScriptRoot "notes-template.md"
$notesTemplate = [System.IO.File]::ReadAllText($templatePath, [System.Text.UTF8Encoding]::new($false))

$chapters = @(
  @("01","introduction","Introduction"),
  @("02","basic-concepts","Fundamental Concepts"),
  @("03","file-io","File I/O: The Universal I/O Model"),
  @("04","file-unbuffered-io","File I/O: Further Details"),
  @("05","file-attributes","File I/O: Metadata"),
  @("06","process-environment","Process Environment"),
  @("07","process-creation","Programs and Processes"),
  @("08","process-users-groups","Process Credentials"),
  @("09","process-execution","Process Execution"),
  @("10","signals-basics","Signals: Basic Concepts"),
  @("11","signal-handling","Signal Handlers"),
  @("12","signal-advanced","Advanced Signal Topics"),
  @("13","timers-sleep","Timers and Sleeping"),
  @("14","file-locking","File Locking"),
  @("15","memory-mapping","Memory Mapping"),
  @("16","shared-libraries","Shared Libraries"),
  @("17","interprocess-comm","Interprocess Communication Overview"),
  @("18","pipes-fifos","Pipes and FIFOs"),
  @("19","message-queues","System V Message Queues"),
  @("20","semaphores","System V Semaphores"),
  @("21","shared-memory","System V Shared Memory"),
  @("22","threads-intro","POSIX Threads"),
  @("23","thread-synchronization","Thread Synchronization"),
  @("24","thread-attributes","Thread Attributes"),
  @("25","thread-scheduling","Thread Scheduling"),
  @("26","thread-specific-data","Thread-Specific Data"),
  @("27","process-groups-sessions","Process Groups, Sessions, and Job Control"),
  @("28","daemon-processes","Daemons"),
  @("29","credentials","Process Credentials (Supplementary Topics)"),
  @("30","process-resources","Process Resources and Limits"),
  @("31","posix-ipc","POSIX IPC Overview"),
  @("32","advanced-message-queues","POSIX Message Queues"),
  @("33","advanced-semaphores","POSIX Semaphores"),
  @("34","advanced-shared-memory","POSIX Shared Memory"),
  @("35","file-systems","File Systems"),
  @("36","directories-links","Directories and Links"),
  @("37","inodes-files","File Attributes and Permissions"),
  @("38","extended-attributes","Extended Attributes"),
  @("39","access-control-lists","Access Control Lists"),
  @("40","monitors","Monitors"),
  @("41","poll-select","I/O Multiplexing: poll and select"),
  @("42","epoll","I/O Multiplexing: epoll"),
  @("43","asynchronous-io","Alternative I/O Models"),
  @("44","memory-allocation","Memory Allocation"),
  @("45","virtual-memory","Virtual Memory"),
  @("46","intro-sockets","Sockets: Introduction"),
  @("47","socket-api","Sockets: Useful APIs"),
  @("48","internet-protocols","Sockets: Internet Protocols"),
  @("49","domain-names","Sockets: Domain Name Resolution"),
  @("50","tcp-sockets","Sockets: TCP Client/Server"),
  @("51","udp-sockets","Sockets: UDP"),
  @("52","socket-options","Sockets: Advanced Topics"),
  @("53","server-design","Sockets: Server Design"),
  @("54","io-multiplexing","Alternative I/O Models (Advanced)"),
  @("55","netlink-sockets","Netlink Sockets"),
  @("56","terminals","Terminals"),
  @("57","termios","Terminal Attributes"),
  @("58","alternative-io-models","Alternative I/O Models (Overview)"),
  @("59","psuedo-terminals","Pseudoterminals"),
  @("60","advanced-ptys","Pseudoterminals (Advanced)"),
  @("61","host-info","Host and Network Information"),
  @("62","program-execution-details","Login Accounting and Process Credentials"),
  @("63","capabilities","Linux Capabilities"),
  @("64","final-summary","Summary and Further Reading")
)

$utf8 = New-Object System.Text.UTF8Encoding $false
foreach ($c in $chapters) {
  $num, $slug, $title = $c
  $dir = Join-Path $tlpi ("chapter-{0}-{1}" -f $num, $slug)
  New-Item -ItemType Directory -Force -Path $dir | Out-Null
  $body = $notesTemplate.Replace("{0}", $num).Replace("{1}", $slug).Replace("{2}", $title)
  [System.IO.File]::WriteAllText((Join-Path $dir ".gitkeep"), "", $utf8)
  [System.IO.File]::WriteAllText((Join-Path $dir "notes.md"), $body, $utf8)
}
Write-Host "Created $($chapters.Count) chapter folders under $tlpi"
