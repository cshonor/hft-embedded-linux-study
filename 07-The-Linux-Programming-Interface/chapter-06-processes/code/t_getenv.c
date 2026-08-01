/* Listing 6-1 style: print all environment variables via environ. */
#include <stdio.h>

extern char **environ;

int main(void) {
    char **ep;
    for (ep = environ; *ep != NULL; ep++)
        puts(*ep);
    return 0;
}
