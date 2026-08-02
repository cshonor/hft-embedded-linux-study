/* Print process capability sets via libcap.
 * cc -Wall -Wextra -o cap_view cap_view.c -lcap && ./cap_view
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/capability.h>
#include <unistd.h>

int main(void)
{
    cap_t caps;
    char *text;

    printf("pid=%ld euid=%ld\n", (long)getpid(), (long)geteuid());

    caps = cap_get_proc();
    if (caps == NULL) {
        perror("cap_get_proc");
        fprintf(stderr, "need libcap headers + link -lcap (e.g. libcap-dev)\n");
        return 1;
    }

    text = cap_to_text(caps, NULL);
    if (text == NULL) {
        perror("cap_to_text");
        cap_free(caps);
        return 1;
    }
    printf("process capabilities:\n  %s\n", text);
    cap_free(text);
    cap_free(caps);

    printf("hex masks: grep ^Cap /proc/%ld/status\n", (long)getpid());
    return 0;
}
