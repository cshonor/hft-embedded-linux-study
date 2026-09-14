/* c8_7_crypt_cost.c —— 8.5：crypt() 的计算成本，就是离线爆破的成本
 *
 * 单核吞吐实测：同一个口令，换不同算法各跑 0.3 秒，换算成「次/秒」。
 * 第二张表由**同一次运行实测到的速率**推算（不是抄来的常数），
 * 回答「为什么 $1$（MD5）必须淘汰、为什么现代发行版默认 $y$」。
 *
 * 编译：cc -O2 -Wall -Wextra -o c8_7_crypt_cost c8_7_crypt_cost.c -lcrypt
 */
#define _DEFAULT_SOURCE
#include <crypt.h>
#include <stdio.h>
#include <time.h>

#define NSCHEME 4

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(void)
{
    const char *schemes[NSCHEME] = {
        "$1$abcdefgh",                   /* MD5-crypt */
        "$5$abcdefgh",                   /* SHA-256-crypt, 5000 轮 */
        "$6$abcdefgh",                   /* SHA-512-crypt, 5000 轮 */
        "$2b$08$abcdefghijklmnopqrstuv", /* bcrypt cost=8 */
    };
    const char *names[NSCHEME] = {
        "$1$（MD5-crypt）", "$5$（SHA-256-crypt）",
        "$6$（SHA-512-crypt）", "$2b$（bcrypt cost=8）",
    };
    double rate[NSCHEME];
    const double budget_ms = 300.0;
    const double wordlist = 1e7;         /* 一千万词的字典，只用于「换算」 */

    puts("算法                            口令数   总耗时      每次耗时     次/秒");
    for (int i = 0; i < NSCHEME; i++) {
        int n = 0;
        double t0 = now_ms();
        while (now_ms() - t0 < budget_ms) {
            crypt("benchmark-pw", schemes[i]);
            n++;
        }
        double dt = now_ms() - t0;
        rate[i] = n * 1000.0 / dt;
        printf("%-30s %-8d %8.1fms %10.1f us %9.0f\n",
               schemes[i], n, dt, dt * 1000.0 / n, rate[i]);
    }

    printf("\n把上面的速率换算成「跑完一千万词字典要多久」（单核，纯算术推算）：\n");
    for (int i = 0; i < NSCHEME; i++) {
        double sec = wordlist / rate[i];
        printf("  %-26s %8.0f 次/秒 -> %10.0f 秒 = %6.2f 小时 = %6.2f 天\n",
               names[i], rate[i], sec, sec / 3600, sec / 86400);
    }

    /* bcrypt 的特点是「成本可调」：每 +1 就翻一倍 */
    double bc12 = rate[3] / 16.0;
    double sec12 = wordlist / bc12;
    printf("  %-26s %8.2f 次/秒 -> %10.0f 秒 = %6.2f 小时 = %6.2f 天\n",
           "$2b$（bcrypt cost=12）", bc12, sec12, sec12 / 3600, sec12 / 86400);

    printf("\n结论：同一份口令字典，MD5-crypt 比 SHA-512-crypt 便宜约 %.0f 倍，\n",
           rate[0] / rate[2]);
    printf("      比 bcrypt(cost=8) 便宜约 %.0f 倍。加一个随机盐只挡预计算（彩虹表），\n",
           rate[0] / rate[3]);
    puts("      挡不住「拿到盐之后逐词试」—— 后者只能靠「每次验证更贵」来拖。");
    puts("      注意 $5$（SHA-256）在本容器上反而比 $6$（SHA-512）慢，");
    puts("      这是 libxcrypt 实现细节，别把「位数越大越慢」当成定律。");
    return 0;
}
