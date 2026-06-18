#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "csonpath_json-c.h"

/* Exhaustive crash-vector tests for the C json-c backend.
 * Each test must either pass gracefully or report a compile error.
 * No SIGSEGV, no ASAN/UBSAN violation.
 */

static int verbose = 1;

#define RUN(test_fn)                            \
    do {                                        \
        if (verbose) { printf("  %s\n", #test_fn); fflush(stdout); } \
        test_fn();                              \
    } while (0)

/* -------------------------------------------------------------------- */
/* 1.  Compile-time rejects / broken paths                               */
/* -------------------------------------------------------------------- */

static void test_compile_empty_path(void)
{
    struct csonpath *p = csonpath_new("");
    assert(p == NULL); /* empty path is invalid */
}

static void test_compile_double_root(void)
{
    struct csonpath *p = csonpath_new("$$");
    assert(p == NULL);
}

static void test_compile_unclosed_bracket(void)
{
    struct csonpath *p = csonpath_new("$[");
    assert(p == NULL);
}

static void test_compile_unclosed_quote_bracket(void)
{
    struct csonpath *p = csonpath_new("$['a]");
    assert(p == NULL);
}

static void test_compile_recursive_descent_no_key(void)
{
    struct csonpath *p = csonpath_new("$..");
    assert(p == NULL);
}

static void test_compile_dot_no_key(void)
{
    struct csonpath *p = csonpath_new("$.");
    assert(p == NULL);
}

static void test_compile_bracket_star_in_find_all(void)
{
    /* [*] is valid as a getter but not as a FIND_ALL path */
    struct csonpath *p = csonpath_new("$..[*]");
    assert(p == NULL);
}

/* -------------------------------------------------------------------- */
/* 2.  NULL value tolerance                                              */
/* -------------------------------------------------------------------- */

static void test_null_value_find_first(void)
{
    struct csonpath *p = csonpath_new("$");
    assert(p);
    CSONPATH_JSON ret = csonpath_find_first(p, NULL);
    /* returns NULL, no crash */
    (void)ret;
    csonpath_destroy(p);
}

static void test_null_value_find_all(void)
{
    struct csonpath *p = csonpath_new("$");
    assert(p);
    CSONPATH_JSON ret = csonpath_find_all(p, NULL);
    /* returns [NULL] -- weird but no crash */
    if (ret) json_object_put(ret);
    csonpath_destroy(p);
}

static void test_null_value_remove(void)
{
    struct csonpath *p = csonpath_new("$.a");
    assert(p);
    int ret = csonpath_remove(p, NULL);
    assert(ret == 0);
    csonpath_destroy(p);
}

static void test_null_value_update_or_create(void)
{
    struct csonpath *p = csonpath_new("$.a");
    struct json_object *jobj = json_tokener_parse("{\"a\":1}");
    assert(p);
    int ret = csonpath_update_or_create(p, NULL, NULL);
    /* returns -1 with error, no crash */
    (void)ret;
    csonpath_destroy(p);
    json_object_put(jobj);
}

/* -------------------------------------------------------------------- */
/* 3.  Index edge cases                                                  */
/* -------------------------------------------------------------------- */

static void test_array_negative_index(void)
{
    /* json-c backend: does negative index wrap around or underflow? */
    struct csonpath *p = csonpath_new("$.a[-1]");
    if (!p) return; /* rejected at compile time is fine */
    struct json_object *jobj = json_tokener_parse("{\"a\":[1,2,3]}");
    CSONPATH_JSON ret = csonpath_find_first(p, jobj);
    (void)ret; /* just don't crash */
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_array_huge_index(void)
{
    struct csonpath *p = csonpath_new("$.a[999999999]");
    if (!p) return;
    struct json_object *jobj = json_tokener_parse("{\"a\":[1,2,3]}");
    CSONPATH_JSON ret = csonpath_find_first(p, jobj);
    assert(ret == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_array_index_on_scalar(void)
{
    struct csonpath *p = csonpath_new("$.a[0]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\":\"hello\"}");
    CSONPATH_JSON ret = csonpath_find_first(p, jobj);
    assert(ret == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_array_index_on_null(void)
{
    struct csonpath *p = csonpath_new("$.a[0]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\":null}");
    CSONPATH_JSON ret = csonpath_find_first(p, jobj);
    assert(ret == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
}

/* -------------------------------------------------------------------- */
/* 4.  Deep / recursive descent on scalars                               */
/* -------------------------------------------------------------------- */

static void test_recursive_descent_on_scalar(void)
{
    struct csonpath *p = csonpath_new("$..a");
    assert(p);
    struct json_object *jobj = json_tokener_parse("\"hello\"");
    CSONPATH_JSON ret = csonpath_find_first(p, jobj);
    assert(ret == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_recursive_descent_on_empty_obj(void)
{
    struct csonpath *p = csonpath_new("$..a");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{}");
    CSONPATH_JSON ret = csonpath_find_first(p, jobj);
    assert(ret == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
}

/* -------------------------------------------------------------------- */
/* 5.  Union edge cases                                                  */
/* -------------------------------------------------------------------- */

static void test_empty_union(void)
{
    /* empty union bracket - should be rejected */
    struct csonpath *p = csonpath_new("$[]");
    assert(p == NULL);
}

static void test_union_mixed_types(void)
{
    struct csonpath *p = csonpath_new("$['a',0]");
    if (!p) return; /* reject is fine */
    struct json_object *jobj = json_tokener_parse("{\"a\":1,\"0\":2}");
    CSONPATH_JSON ret = csonpath_find_first(p, jobj);
    (void)ret;
    csonpath_destroy(p);
    json_object_put(jobj);
}

/* -------------------------------------------------------------------- */
/* 6.  Object getter on non-object                                       */
/* -------------------------------------------------------------------- */

static void test_get_obj_on_array(void)
{
    struct csonpath *p = csonpath_new("$.a");
    assert(p);
    struct json_object *jobj = json_tokener_parse("[1,2,3]");
    CSONPATH_JSON ret = csonpath_find_first(p, jobj);
    assert(ret == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_get_obj_on_scalar(void)
{
    struct csonpath *p = csonpath_new("$.a");
    assert(p);
    struct json_object *jobj = json_tokener_parse("\"hello\"");
    CSONPATH_JSON ret = csonpath_find_first(p, jobj);
    assert(ret == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
}

/* -------------------------------------------------------------------- */
/* 7.  Remove / update on array                                                          */
/* -------------------------------------------------------------------- */

static void test_remove_array_element(void)
{
    struct csonpath *p = csonpath_new("$.a[1]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\":[1,2,3]}");
    int ret = csonpath_remove(p, jobj);
    (void)ret; /* just don't crash */
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_update_array_element(void)
{
    struct csonpath *p = csonpath_new("$.a[1]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\":[1,2,3]}");
    struct json_object *val = json_object_new_int(42);
    int ret = csonpath_update_or_create(p, jobj, val);
    (void)ret;
    csonpath_destroy(p);
    json_object_put(jobj);
    json_object_put(val);
}

/* -------------------------------------------------------------------- */
/* 8.  csonpath_destroy / set_path double-free style                     */
/* -------------------------------------------------------------------- */

static void test_set_path_invalid_then_valid(void)
{
    struct csonpath *p = csonpath_new("$.a");
    assert(p);
    /* set_path with invalid string destroys old p and returns NULL */
    struct csonpath *q = csonpath_set_path(p, "$$");
    assert(q == NULL);
    /* p is already freed by set_path; don't touch it again */
}

/* -------------------------------------------------------------------- */
/* Main                                                                */
/* -------------------------------------------------------------------- */

int main(void)
{
    printf("=== Crash-vector C tests ===\n"); fflush(stdout);

    printf("\n-- Compile-time rejects --\n"); fflush(stdout);
    RUN(test_compile_empty_path);
    RUN(test_compile_double_root);
    RUN(test_compile_unclosed_bracket);
    RUN(test_compile_unclosed_quote_bracket);
    RUN(test_compile_recursive_descent_no_key);
    RUN(test_compile_dot_no_key);
    RUN(test_compile_bracket_star_in_find_all);

    printf("\n-- NULL value tolerance --\n"); fflush(stdout);
    RUN(test_null_value_find_first);
    RUN(test_null_value_find_all);
    RUN(test_null_value_remove);
    RUN(test_null_value_update_or_create);

    printf("\n-- Index edge cases --\n"); fflush(stdout);
    RUN(test_array_negative_index);
    RUN(test_array_huge_index);
    RUN(test_array_index_on_scalar);
    RUN(test_array_index_on_null);

    printf("\n-- Recursive descent on scalars --\n"); fflush(stdout);
    RUN(test_recursive_descent_on_scalar);
    RUN(test_recursive_descent_on_empty_obj);

    printf("\n-- Union edge cases --\n"); fflush(stdout);
    RUN(test_empty_union);
    RUN(test_union_mixed_types);

    printf("\n-- Getter on non-object --\n"); fflush(stdout);
    RUN(test_get_obj_on_array);
    RUN(test_get_obj_on_scalar);

    printf("\n-- Array remove / update --\n"); fflush(stdout);
    RUN(test_remove_array_element);
    RUN(test_update_array_element);

    printf("\n-- set_path --\n"); fflush(stdout);
    RUN(test_set_path_invalid_then_valid);

    printf("\n=== All crash-vector tests passed ===\n"); fflush(stdout);
    return 0;
}
