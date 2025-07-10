#include <assert.h>
#include <stdio.h>

#include "jtest.h"
#include "instr.h"
#include "util.h"
#include "main.h"

static const int key_to_idx[256] = {
    ['a'] = 0, ['f'] = 1, ['b'] = 2, ['c'] = 3, ['d'] = 4, ['e'] = 5, ['h'] = 6, ['l'] = 7
};

void kvs_reset(struct kvs *kvs) {
    assert (kvs);
    vec_reset((struct vec*) kvs->fields);
}

int kvs_append(struct kvs *kvs, struct gbs_kv *val) {
    assert (kvs);
    return vec_append((struct vec*) &kvs->fields, sizeof(struct gbs_kv), val);
}

int kvs_pop(struct kvs *kvs, struct gbs_kv *val) {
    assert (kvs && kvs->fields);
    return vec_pop((struct vec*) &kvs->fields, sizeof(struct gbs_kv), val);
}

void rams_reset(struct rams *rams) {
    assert (rams);
    vec_reset((struct vec*) rams->states);
}

int rams_append(struct rams *rams, struct ram_state *val) {
    assert (rams);
    return vec_append((struct vec*) &rams->states, sizeof(struct ram_state), val);
}

int rams_pop(struct rams *rams, struct ram_state *val) {
    assert (rams && rams->states);
    return vec_pop((struct vec*) &rams->states, sizeof(struct ram_state), val);
}

struct gbstate empty_gbstate(){
    struct gbstate state = {0};
    return state;
}

struct gbstate test_gbstate_to_gbstate(struct test_gbstate s){
    struct gbstate to_ret = empty_gbstate();
    struct rams r = s.rams;
    for(int i = 0; i < r.len; i++){
        struct ram_state rs = r.states[i];
        to_ret.ram[rs.pos] = rs.val;
    }
    struct kvs k = s.kvs;
    for(int i = 0; i < k.len; i++){
        struct gbs_kv kv = k.fields[i];
        if(strcmp(kv.k, "sp") == 0){
            to_ret.sp = kv.v;
        } else if (strcmp(kv.k, "pc") == 0){
            to_ret.pc = kv.v;
        } else if (strcmp(kv.k, "ime") == 0){
        } else {
            uint8_t reg_idx = key_to_idx[kv.k[0]];
            to_ret.reg[reg_idx] = kv.v;
        }
    }
    return to_ret;
}

int verify_gbstate_with_test(struct test_gbstate s, struct gbstate s_hat){
    struct rams r = s.rams;
    int mismatches = 0;
    for(int i = 0; i < r.len; i++){
        struct ram_state rs = r.states[i];
        mismatches += (s_hat.ram[rs.pos] != rs.val);
    }
    struct kvs k = s.kvs;
    for(int i = 0; i < k.len; i++){
        struct gbs_kv kv = k.fields[i];
        if(strcmp(kv.k, "sp") == 0){
            mismatches += (s_hat.sp != kv.v);
        } else if (strcmp(kv.k, "pc") == 0){
            mismatches += (s_hat.pc != kv.v);
        } else if (strcmp(kv.k, "ime") == 0){
        } else {
            uint8_t reg_idx = key_to_idx[kv.k[0]];
            mismatches += (s_hat.reg[reg_idx] != kv.v);
        }
    }
    return mismatches;
}

int run_sm83_test(struct sm83_test t){
    struct gbstate s = test_gbstate_to_gbstate(t.initial);
    step(&s);
    return verify_gbstate_with_test(t.final, s); // 0 = success
}

void sm83_test_dump(struct sm83_test *tests, size_t tests_len) {
    for (size_t i = 0; i < tests_len && i < 10; i++) {
        printf("Name: %s\n", tests[i].name);
        printf("\tInitial KVs:");
        for (size_t j = 0; j < tests[i].initial.kvs.len; j++) {
            struct gbs_kv kv = tests[i].initial.kvs.fields[j];
            printf("[%s, %zu]", kv.k, kv.v);
        }
        printf("\n");
        printf("\tInitial RAM States:");
        for (size_t j = 0; j < tests[i].initial.rams.len; j++) {
            struct ram_state rs = tests[i].initial.rams.states[j];
            printf("[%zu, %zu]", rs.pos, rs.val);
        }
        printf("\n");
        printf("\tFinal KVs:");
        for (size_t j = 0; j < tests[i].final.kvs.len; j++) {
            struct gbs_kv kv = tests[i].final.kvs.fields[j];
            printf("[%s, %zu]", kv.k, kv.v);
        }
        printf("\n");
        printf("\tFinal RAM States:");
        for (size_t j = 0; j < tests[i].initial.rams.len; j++) {
            struct ram_state rs = tests[i].final.rams.states[j];
            printf("[%zu, %zu]", rs.pos, rs.val);
        }
        printf("\n\n");
    }
}
