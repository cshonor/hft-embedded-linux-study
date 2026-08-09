#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static struct termios saved;
static int have_saved;

static void restore_tty(void)
{
    if (have_saved)
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved);
}

static void on_signal(int sig)
{
    (void)sig;
    restore_tty();
    /* re-raise default after restore */
    signal(sig, SIG_DFL);
    raise(sig);
}

int main(void)
{
    struct termios t;
    char buf[128];
    ssize_t n;

    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "stdin is not a tty\n");
        return 1;
    }

    if (tcgetattr(STDIN_FILENO, &saved) == -1) {
        perror("tcgetattr");
        return 1;
    }
    have_saved = 1;
    atexit(restore_tty);
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    t = saved;
    t.c_lflag &= ~(tcflag_t)ECHO; /* password-style: no echo */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) == -1) {
        perror("tcsetattr");
        return 1;
    }

    printf("type a line (echo off): ");
    fflush(stdout);
    n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
    restore_tty();
    have_saved = 0;

    if (n <= 0) {
        printf("\n");
        return 1;
    }
    buf[n] = '\0';
    printf("\ngot: %s", buf);
    return 0;
}
