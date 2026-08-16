# Ch19 · Variations in control flow（控制流的变化）

> **Level 3 · 深入** · 策略：**🟡 略读**
> 《Modern C》第三版（C23 版）· Jens Gustedt · 免费版：gustedt.gitlabpages.inria.fr/modern-c/

## 小节清单（骨架，待充实）

- [ ] 顺序执行与副作用
- [ ] 短跳转
- [ ] 函数与尾调用
- [ ] **长跳转 setjmp/longjmp**
- [ ] **信号处理器 signal/sigaction 限制（async-signal-safe）**

## HFT / DPDK 关联

信号处理器里只能用 async-signal-safe 函数——HFT 行情进程的自救/重启逻辑要懂；longjmp 跳出错误恢复路径

## 自测题（待补）

<details><summary>1. （待补充）</summary>

（待补充）
</details>

---

> 本文件为章节骨架。读书时按仓库体例充实：概念 + 代码 + HFT 关联 + 自测题（`<details>` 折叠答案）。
