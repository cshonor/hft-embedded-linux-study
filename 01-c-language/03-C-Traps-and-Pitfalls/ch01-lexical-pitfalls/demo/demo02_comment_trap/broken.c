/* 此文件故意演示注释嵌套陷阱 — 不要直接编译 main.c
 * 正确屏蔽方式见 demo02_comment_trap/README.md
 */
#if 0
/* outer
int a;
/* inner */
int b;
*/
#endif

int main(void) { return 0; }
