#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

/* background=1：fork 后不等待（后台 `&`）。僵尸由 SIGCHLD 收割。 */
void run_pipeline(struct cmd *cmds, int n, int background);

#endif
