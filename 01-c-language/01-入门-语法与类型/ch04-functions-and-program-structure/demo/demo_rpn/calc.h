#ifndef CALC_H
#define CALC_H

#define MAX_STACK 100
#define MAX_TOKEN 100

extern int stack_ptr;

#include "stack.h"

int getop(char buf[]);

#endif
