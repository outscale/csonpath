#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "csonpath_json-c.h"

#define N_OBJECTS 1000
#define DEPTH 10
#define KEY_LEN 100
#define ITERS 1000

static double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static json_object *build_data(void)
{
    json_object *root = json_object_new_object();
    char long_key[KEY_LEN + 1];
    int i, j;

    memset(long_key, 'k', KEY_LEN);
    long_key[KEY_LEN] = '\0';

    for (i = 0; i < N_OBJECTS; ++i) {
        json_object *obj = json_object_new_object();
        json_object *cur = obj;
        char obj_name[16];

        for (j = 0; j < DEPTH; ++j) {
            json_object *next = json_object_new_object();
            json_object_object_add(cur, long_key, next);
            cur = next;
        }
        json_object_object_add(cur, "leaf", json_object_new_string("deep value"));

        json_object *arr = json_object_new_array();
        for (j = 0; j < 5; ++j) {
            json_object *item = json_object_new_object();
            json_object *inner = json_object_new_object();
            json_object_object_add(inner, "kk", json_object_new_string("array value"));
            json_object_object_add(item, "k", inner);
            json_object_array_add(arr, item);
        }
        json_object_object_add(cur, "arr", arr);

        snprintf(obj_name, sizeof(obj_name), "obj%d", i);
        json_object_object_add(root, obj_name, obj);
    }

    return root;
}

static void build_dot_path(char *buf, size_t buf_size, const char *long_key)
{
    int i;
    snprintf(buf, buf_size, "$.obj0");
    for (i = 0; i < DEPTH; ++i) {
        strncat(buf, ".", buf_size - strlen(buf) - 1);
        strncat(buf, long_key, buf_size - strlen(buf) - 1);
    }
    strncat(buf, ".leaf", buf_size - strlen(buf) - 1);
}

static void build_bracket_path(char *buf, size_t buf_size, const char *long_key)
{
    int i;
    snprintf(buf, buf_size, "$['obj0']");
    for (i = 0; i < DEPTH; ++i) {
        strncat(buf, "['", buf_size - strlen(buf) - 1);
        strncat(buf, long_key, buf_size - strlen(buf) - 1);
        strncat(buf, "']", buf_size - strlen(buf) - 1);
    }
    strncat(buf, "['leaf']", buf_size - strlen(buf) - 1);
}

static void bench_query(const char *name, const char *query, json_object *root)
{
    struct csonpath *p = csonpath_new(query);
    size_t count = 0;
    double start, elapsed;
    int i;

    if (!p) {
        fprintf(stderr, "failed to compile: %s\n", query);
        return;
    }

    start = now_seconds();
    for (i = 0; i < ITERS; ++i) {
        struct json_object *ret = csonpath_find_all(p, root);
        count = ret ? json_object_array_length(ret) : 0;
        json_object_put(ret);
    }
    elapsed = now_seconds() - start;

    printf("Query: %s\n", name);
    printf("Path: %s\n", query);
    printf("Results: %zu, in %d loop, Time: %.6f seconds\n\n", count, ITERS, elapsed);

    csonpath_destroy(p);
}

int main(void)
{
    json_object *root = build_data();
    char long_key[KEY_LEN + 1];
    char path_dot[4096];
    char path_bracket[4096];
    char path_recursive[KEY_LEN + 5];

    memset(long_key, 'k', KEY_LEN);
    long_key[KEY_LEN] = '\0';

    build_dot_path(path_dot, sizeof(path_dot), long_key);
    build_bracket_path(path_bracket, sizeof(path_bracket), long_key);
    snprintf(path_recursive, sizeof(path_recursive), "$..%s", long_key);

    bench_query("dot long/deep", path_dot, root);
    bench_query("bracket long/deep", path_bracket, root);
    bench_query("recursive descent long key", path_recursive, root);
    bench_query("wildcard array", "$..arr[*].k.kk", root);

    json_object_put(root);
    return 0;
}
