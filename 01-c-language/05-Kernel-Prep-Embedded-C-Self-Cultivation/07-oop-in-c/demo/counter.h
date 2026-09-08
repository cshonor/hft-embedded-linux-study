#ifndef COUNTER_H
#define COUNTER_H

typedef struct counter counter_t;

counter_t *counter_create(int initial);
void counter_destroy(counter_t *c);
int counter_get(const counter_t *c);
void counter_inc(counter_t *c);

#endif
