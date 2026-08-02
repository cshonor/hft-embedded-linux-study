/* Pass an exit value via return / pthread_exit; join retrieves it.
 * cc -Wall -Wextra -pthread -o thread_exit_retval thread_exit_retval.c
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *worker(void *arg)
{
    int n = *(int *)arg;
    /* Heap so value outlives the thread stack after exit */
    int *ret = malloc(sizeof(*ret));
    if (ret == NULL)
        pthread_exit(NULL);
    *ret = n * 2;
    pthread_exit(ret); /* same as return ret; */
}

int main(void)
{
    pthread_t t;
    int arg = 21;
    void *retval;
    int err;

    err = pthread_create(&t, NULL, worker, &arg);
    if (err != 0) {
        fprintf(stderr, "create: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    err = pthread_join(t, &retval);
    if (err != 0) {
        fprintf(stderr, "join: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    if (retval == NULL) {
        fprintf(stderr, "no retval\n");
        return EXIT_FAILURE;
    }
    printf("joined retval=%d\n", *(int *)retval);
    free(retval);
    return 0;
}
