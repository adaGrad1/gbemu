#ifndef JTEST_H_
#define JTEST_H_
#include <stdlib.h>

struct gbs_kv {
    // NOTE: Not all the initial state values are registers
    char *k;
    size_t v;
};

struct ram_state {
    size_t pos;
    size_t val;
};

struct kvs {
    struct gbs_kv *fields;
    size_t len;
    size_t cap;
};

struct rams {
    struct ram_state *states;
    size_t len;
    size_t cap;
};

struct test_gbstate {
    struct kvs kvs;
    struct rams rams;
};

struct sm83_test {
    char *name;
    struct test_gbstate initial;
    struct test_gbstate final;
};

void kvs_reset(struct kvs *kvs);
int kvs_append(struct kvs *kvs, struct gbs_kv *val);
int kvs_pop(struct kvs *kvs, struct gbs_kv *val);

void rams_reset(struct rams *rams);
int rams_append(struct rams *rams, struct ram_state *val);
int rams_pop(struct rams *rams, struct ram_state *val);

void sm83_test_dump(struct sm83_test *tests, size_t tests_len);
int run_sm83_test(struct sm83_test t);
#endif // JTEST_H_
