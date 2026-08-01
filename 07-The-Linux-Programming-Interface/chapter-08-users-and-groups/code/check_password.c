/* Listing 8-1 style: verify password via getspnam + crypt (needs root / CAP_DAC_READ_SEARCH). */
#define _XOPEN_SOURCE 700
#include <errno.h>
#include <shadow.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static void wipe(char *s) {
    if (s != NULL)
        memset(s, 0, strlen(s));
}

int main(int argc, char *argv[]) {
    char *username;
    char *password;
    char *encrypted;
    struct spwd *spwd;
    struct termios tp, save;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s username\n", argv[0]);
        return 1;
    }
    username = argv[1];

    spwd = getspnam(username);
    if (spwd == NULL) {
        if (errno == EACCES)
            fprintf(stderr, "getspnam: need privilege to read shadow\n");
        else
            perror("getspnam");
        return 1;
    }

    printf("Password: ");
    fflush(stdout);
    if (tcgetattr(STDIN_FILENO, &tp) == -1) {
        perror("tcgetattr");
        return 1;
    }
    save = tp;
    tp.c_lflag &= ~(ECHO);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tp) == -1) {
        perror("tcsetattr");
        return 1;
    }
    password = NULL;
    size_t n = 0;
    if (getline(&password, &n, stdin) == -1) {
        perror("getline");
        tcsetattr(STDIN_FILENO, TCSANOW, &save);
        return 1;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &save);
    putchar('\n');
    if (password[0] != '\0' && password[strlen(password) - 1] == '\n')
        password[strlen(password) - 1] = '\0';

    encrypted = crypt(password, spwd->sp_pwdp);
    wipe(password);
    free(password);
    if (encrypted == NULL) {
        perror("crypt");
        return 1;
    }

    if (strcmp(encrypted, spwd->sp_pwdp) == 0)
        printf("Password correct\n");
    else
        printf("Password incorrect\n");
    return 0;
}
