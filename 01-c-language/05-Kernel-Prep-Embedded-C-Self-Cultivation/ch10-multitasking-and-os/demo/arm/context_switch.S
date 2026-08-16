/*
 * ARM 上下文切换骨架（A32，教学参考）
 * 真机/FreeRTOS 在 PendSV 中执行类似序列：
 *   stmfd sp!, {r4-r11, lr}
 *   str sp, [r0]          @ 保存旧任务 SP 到 TCB
 *   ldr sp, [r1]          @ 加载新任务 SP
 *   ldmfd sp!, {r4-r11, pc}
 */
    .text
    .global context_switch
context_switch:
    @ void context_switch(TCB **from, TCB **to);
    push    {r4-r11, lr}
    @ ... 保存/恢复 SP 由 C 侧 TCB 完成
    pop     {r4-r11, pc}
