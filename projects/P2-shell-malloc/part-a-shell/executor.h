#ifndef EXECUTOR_H
#define EXECUTOR_H

/* Run external command (fork + execvp + waitpid). */
void run_external(char **argv);

#endif
