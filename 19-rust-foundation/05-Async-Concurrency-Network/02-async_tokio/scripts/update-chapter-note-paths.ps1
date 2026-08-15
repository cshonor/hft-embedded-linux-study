# 更新 async_tokio 本章学习笔记 中的旧 demo 路径
$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..")

$map = @{
  "ch03_custom_task_queue/本章学习笔记.md" = @(
    @("``ch03_join_macro_flume.rs``", "``3.6-custom-join-macro/3.6-custom-join-macro-flume-demo.rs``"),
    @("cargo run --example ch03_join_macro_flume", "``cargo run --example ch03_join_macro_flume``（源码：`3.6-custom-join-macro/3.6-custom-join-macro-flume-demo.rs`）")
  )
  "ch04_network_io_runtime/本章学习笔记.md" = @(
    @("``ch04_mio_poll_listener.rs``", "``4.5-mio-poll/4.5-mio-poll-listener-demo.rs``")
  )
  "ch05_coroutines_generators/本章学习笔记.md" = @(
    @("``ch05_resume_state_machine.rs``", "``5.3-simple-generator/5.3-simple-generator-state-machine-demo.rs``")
  )
  "ch06_reactive_async_streams/本章学习笔记.md" = @(
    @("``ch06_callback.rs``", "``6.4-user-input-via-callbacks/6.4-user-input-via-callbacks-demo.rs``"),
    @("``ch06_heater_display.rs``", "``6.2-building-display-observer/6.2-building-display-observer-demo.rs``"),
    @("``6.1-heater-system/``", "``6.1-basic-reactive-system-and-subjects/``")
  )
  "ch07_tokio_graceful_shutdown/本章学习笔记.md" = @(
    @("``ch07_local_pool_pinned.rs``", "``7.2-processing-tasks-with-local-pools/7.2-processing-tasks-with-local-pools-demo.rs``"),
    @("``ch07_runtime_builder_hooks.rs``", "``7.1-building-a-runtime/7.1-building-a-runtime-demo.rs``"),
    @("``ch07_ctrl_c_shutdown.rs``", "``7.4-graceful-shutdowns/7.4-graceful-shutdowns-ctrl-c-demo.rs``")
  )
  "ch08_actor_model/本章学习笔记.md" = @(
    @("``ch08_actor_resp_channel.rs``", "``8.1-building-basic-actor/8.1-building-basic-actor-demo.rs``")
  )
  "ch09_async_design_patterns/本章学习笔记.md" = @(
    @("``ch09_retry_backoff.rs``", "``9.5-retry-pattern/9.5-retry-pattern-backoff-demo.rs``"),
    @("``9.1-isolated-module/``", "``9.1-building-isolated-module/``")
  )
  "ch10_dependency_free_async_server/本章学习笔记.md" = @(
    @("``ch10_std_tcp_nonblocking.rs``", "``10.2-building-std-async-runtime/10.2-building-std-async-runtime-tcp-nonblocking-demo.rs``"),
    @("``ch10_noop_waker_block_on.rs``", "``10.2-building-std-async-runtime/10.2-building-std-async-runtime-noop-waker-demo.rs``"),
    @("``10.1-basics/``", "``10.1-setting-up-basics/``"),
    @("``10.2-std-runtime/``", "``10.2-building-std-async-runtime/``")
  )
  "ch01_async_intro/本章学习笔记.md" = @(
    @("同目录 ``demo.rs``", "``1.1-what-is-async/1.1-what-is-async-stdlib-demo.rs``")
  )
  "ch02_async_rust_core/本章学习笔记.md" = @(
    @("``cargo run --example ch02_counter_future``", "``rustc`` / 主工程挂接 `2.1-future-trait/2.1-future-trait-counter-demo.rs`"),
    @("同目录 ``demo.rs``", "``2.1-future-trait/2.1-future-trait-stdlib-demo.rs``（若有）")
  )
}

foreach ($rel in $map.Keys) {
  $path = Join-Path $root $rel
  if (-not (Test-Path $path)) { continue }
  $text = Get-Content $path -Raw -Encoding UTF8
  foreach ($pair in $map[$rel]) { $text = $text.Replace($pair[0], $pair[1]) }
  Set-Content $path $text -Encoding UTF8 -NoNewline
  Write-Host "Updated $rel"
}

Write-Host "Done."
