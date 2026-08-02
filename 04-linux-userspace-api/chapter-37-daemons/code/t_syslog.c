/* syslog API smoke test (runs in foreground).
 * cc -Wall -Wextra -o t_syslog t_syslog.c && ./t_syslog
 */
#include <syslog.h>
#include <unistd.h>

int main(void)
{
    openlog("tlpi-ch37", LOG_PID | LOG_NDELAY, LOG_USER);
    syslog(LOG_INFO, "hello from pid %ld", (long)getpid());
    syslog(LOG_WARNING, "sample warning level");
    closelog();
    return 0;
}
