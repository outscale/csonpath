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

static void test_recursive_descent_into_array(void)
{
    /* $..a on an array of objects must enter the array branch of
     * csonpath_do_dotdot (line 170). */
    struct csonpath *p = csonpath_new("$..a");
    assert(p);
    struct json_object *jobj = json_tokener_parse("[{\"a\": 1}, {\"a\": 2}]");
    CSONPATH_JSON ret = csonpath_find_all(p, jobj);
    assert(ret && json_object_array_length(ret) == 2);
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

static void test_union_find_all(void)
{
    /* UNION_END -> CSONPATH_DO_GET_ALL_OUT in find_all mode. */
    struct csonpath *p = csonpath_new("$['a','b']");
    assert(p);
    struct json_object *jobj = json_tokener_parse(
        "{\"a\": 1, \"b\": 2, \"c\": 3}");
    CSONPATH_JSON ret = csonpath_find_all(p, jobj);
    assert(ret && json_object_array_length(ret) == 2);
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
/* 6b. Filter and range edge cases                                       */
/* -------------------------------------------------------------------- */

static void test_filter_on_scalar(void)
{
    /* Filter operand on a non-array must hit CSONPATH_DO_FILTER_OUT. */
    struct csonpath *p = csonpath_new("$.a[?(@.x)]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\": 1}");
    CSONPATH_JSON ret = csonpath_find_first(p, jobj);
    assert(ret == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_range_on_non_array(void)
{
    struct csonpath *p = csonpath_new("$.a[1:2]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\": {\"x\": 1}}");
    CSONPATH_JSON ret = csonpath_find_first(p, jobj);
    assert(ret == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_range_basic(void)
{
    /* A non-empty range must enter the loop body (CSONPATH_DO_FOREACH_PRE_SET). */
    struct csonpath *p = csonpath_new("$.a[1:2]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\": [10, 20, 30]}");
    CSONPATH_JSON ret = csonpath_find_first(p, jobj);
    assert(ret && json_object_get_int(ret) == 20);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_range_find_all(void)
{
    struct csonpath *p = csonpath_new("$.a[0:2]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\": [10, 20, 30, 40]}");
    CSONPATH_JSON ret = csonpath_find_all(p, jobj);
    assert(ret && json_object_array_length(ret) == 2);
    json_object_put(ret);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_range_update_or_create(void)
{
    struct csonpath *p = csonpath_new("$.a[0:2]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\": [10, 20, 30]}");
    struct json_object *val = json_object_new_int(99);
    int ret = csonpath_update_or_create(p, jobj, val);
    json_object_put(val);
    assert(ret == 2);
    struct json_object *arr = json_object_object_get(jobj, "a");
    assert(json_object_get_int(json_object_array_get_idx(arr, 0)) == 99);
    assert(json_object_get_int(json_object_array_get_idx(arr, 1)) == 99);
    assert(json_object_get_int(json_object_array_get_idx(arr, 2)) == 30);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_range_remove(void)
{
    struct csonpath *p = csonpath_new("$.a[0:1]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\": [10, 20, 30]}");
    int ret = csonpath_remove(p, jobj);
    assert(ret == 1);
    struct json_object *arr = json_object_object_get(jobj, "a");
    assert(json_object_array_length(arr) == 3);
    assert(json_object_array_get_idx(arr, 0) == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
}

/* -------------------------------------------------------------------- */
/* 7.  Remove / update on array                                          */
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

static void test_remove_array_all(void)
{
    struct csonpath *p = csonpath_new("$.a[*]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\":[1,2,3]}");
    int ret = csonpath_remove(p, jobj);
    assert(ret == 3); /* one deletion per element */
    /* json-c backend replaces array elements with NULL rather than
     * shifting the array, so length stays 3 but values are NULL. */
    struct json_object *arr = json_object_object_get(jobj, "a");
    assert(json_object_array_length(arr) == 3);
    for (size_t i = 0; i < 3; ++i)
        assert(json_object_array_get_idx(arr, i) == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_remove_array_out_of_bounds(void)
{
    struct csonpath *p = csonpath_new("$.a[5]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\":[1,2,3]}");
    int ret = csonpath_remove(p, jobj);
    assert(ret < 1); /* nothing deleted */
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
    json_object_put(val);
    assert(ret == 1);
    assert(json_object_get_int(
        json_object_array_get_idx(
            json_object_object_get(jobj, "a"), 1)) == 42);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_update_array_out_of_bounds_gaps(void)
{
    struct csonpath *p = csonpath_new("$.a[5]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\":[1,2,3]}");
    struct json_object *val = json_object_new_int(42);
    int ret = csonpath_update_or_create(p, jobj, val);
    json_object_put(val);
    assert(ret == 1);
    struct json_object *arr = json_object_object_get(jobj, "a");
    assert(json_object_array_length(arr) == 6);
    assert(json_object_array_get_idx(arr, 3) == NULL);
    assert(json_object_array_get_idx(arr, 4) == NULL);
    assert(json_object_get_int(json_object_array_get_idx(arr, 5)) == 42);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_update_array_big_index(void)
{
    /* GET_ARRAY_BIG with an existing index must reach
     * CSONPATH_DO_POST_FIND_ARRAY. */
    struct csonpath *p = csonpath_new("$.a[100]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\": [0]}");
    struct json_object *val = json_object_new_int(42);
    int ret = csonpath_update_or_create(p, jobj, val);
    json_object_put(val);
    assert(ret == 1);
    struct json_object *arr = json_object_object_get(jobj, "a");
    assert(json_object_array_length(arr) == 101);
    assert(json_object_get_int(json_object_array_get_idx(arr, 100)) == 42);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_update_array_element_by_subpath_index(void)
{
    /* GET_SUBPATH numeric branch with a valid index must reach
     * CSONPATH_DO_POST_FIND_ARRAY in update_or_create mode. */
    struct csonpath *p = csonpath_new("$.a[$.b]");
    assert(p);
    struct json_object *jobj = json_tokener_parse(
        "{\"a\":[10,20,30],\"b\":1}");
    struct json_object *val = json_object_new_int(42);
    int ret = csonpath_update_or_create(p, jobj, val);
    json_object_put(val);
    assert(ret == 1);
    assert(json_object_get_int(
        json_object_array_get_idx(
            json_object_object_get(jobj, "a"), 1)) == 42);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_remove_array_element_by_subpath_index(void)
{
    /* GET_SUBPATH numeric branch with a valid index must reach
     * CSONPATH_DO_POST_FIND_ARRAY (remove mode). */
    struct csonpath *p = csonpath_new("$.a[$.b]");
    assert(p);
    struct json_object *jobj = json_tokener_parse(
        "{\"a\":[10,20,30],\"b\":1}");
    int ret = csonpath_remove(p, jobj);
    assert(ret == 1);
    struct json_object *arr = json_object_object_get(jobj, "a");
    assert(json_object_array_length(arr) == 3);
    assert(json_object_array_get_idx(arr, 1) == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
}

/* -------------------------------------------------------------------- */
/* 8.  Recursive descent remove                                          */
/* -------------------------------------------------------------------- */

static void test_remove_recursive_descent(void)
{
    struct csonpath *p = csonpath_new("$..a");
    assert(p);
    struct json_object *jobj = json_tokener_parse(
        "{\"x\": {\"a\": 1}, \"y\": {\"a\": 2}}");
    int ret = csonpath_remove(p, jobj);
    assert(ret == 2);
    struct json_object *x = json_object_object_get(jobj, "x");
    struct json_object *y = json_object_object_get(jobj, "y");
    assert(x && json_object_object_length(x) == 0);
    assert(y && json_object_object_length(y) == 0);
    csonpath_destroy(p);
    json_object_put(jobj);
}

/* -------------------------------------------------------------------- */
/* 9.  Mutation during callback iteration                                */
/* -------------------------------------------------------------------- */

static void json_c_callback_delete_next(struct json_object *ctx,
                                         struct csonpath_child_info *child_info,
                                         struct json_object *val, void *udata)
{
    (void)child_info; (void)val; (void)udata;
    /* delete the key 'c' regardless of which key we are on */
    json_object_object_del(ctx, "c");
}

static void test_callback_mutation_next_key(void)
{
    struct csonpath *p = csonpath_new("$.a.*");
    struct json_object *jobj = json_tokener_parse(
        "{\"a\":{\"b\":1,\"c\":2,\"d\":3}}");
    int ret;

    assert(p);
    assert(jobj);
    /* deleting an entry that has NOT been visited yet must not crash.
     * Number of callbacks is not deterministic because deleting an entry
     * may corrupt the iterator's cached next pointer (UAF). */
    ret = csonpath_callback(p, jobj, json_c_callback_delete_next, NULL);
    (void)ret;
    assert(json_object_object_get(
        json_object_object_get(jobj, "a"), "c") == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
}

/* -------------------------------------------------------------------- */
/* 10.  Deep nesting / stack stress                                      */
/* -------------------------------------------------------------------- */

static struct json_object *make_nested(int depth)
{
    if (depth <= 0)
        return json_object_new_int(1);
    struct json_object *o = json_object_new_object();
    json_object_object_add(o, "a", make_nested(depth - 1));
    return o;
}

static void test_deep_recursive_descent(void)
{
    /* 200 levels deep — recursive descent must not stack-overflow */
    struct json_object *jobj = make_nested(200);
    struct csonpath *p = csonpath_new("$..a");
    CSONPATH_JSON ret;

    assert(p);
    /* find_first returns the first match (top-level nested object) */
    ret = csonpath_find_first(p, jobj);
    assert(ret != NULL);
    assert(json_object_is_type(ret, json_type_object));
    /* find_all should collect every nested level (200 items) */
    ret = csonpath_find_all(p, jobj);
    assert(ret != NULL);
    assert(json_object_array_length(ret) == 200);
    json_object_put(ret);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void test_wide_array_get_all(void)
{
    /* array with 64k elements — GET_ALL must return every element */
    struct json_object *arr = json_object_new_array();
    for (int i = 0; i < 65536; ++i)
        json_object_array_add(arr, json_object_new_int(i));
    struct json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "a", arr);
    struct csonpath *p = csonpath_new("$.a[*]");
    CSONPATH_JSON ret;

    assert(p);
    ret = csonpath_find_all(p, jobj);
    assert(ret);
    assert(json_object_array_length(ret) == 65536);
    json_object_put(ret);
    csonpath_destroy(p);
    json_object_put(jobj);
}

/* -------------------------------------------------------------------- */
/* 11. csonpath_destroy / set_path double-free style                     */
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
/* 12. find_first on a broken path object                               */
/* -------------------------------------------------------------------- */

static void test_find_first_on_broken_path(void)
{
    struct csonpath *p = csonpath_new_ex("$$", CSONPATH_NO_DETROY);
    assert(p);
    assert(p->compile_error);
    struct json_object *jobj = json_tokener_parse("{}");
    CSONPATH_JSON ret = csonpath_find_first(p, jobj);
    assert(ret == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
}

/* -------------------------------------------------------------------- */
/* 13. Callback and update_or_create_callback                           */
/* -------------------------------------------------------------------- */

static int callback_count = 0;

static void json_c_callback_count(struct json_object *ctx,
                                  struct csonpath_child_info *child_info,
                                  struct json_object *val, void *udata)
{
    (void)ctx; (void)child_info; (void)val; (void)udata;
    callback_count++;
}

static void test_callback_basic(void)
{
    callback_count = 0;
    struct csonpath *p = csonpath_new("$.a[*]");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{\"a\":[1,2,3]}");
    int ret = csonpath_callback(p, jobj, json_c_callback_count, NULL);
    assert(ret == 3);
    assert(callback_count == 3);
    csonpath_destroy(p);
    json_object_put(jobj);
}

static void json_c_callback_set_42(struct json_object *ctx,
                                   struct csonpath_child_info *child_info,
                                   struct json_object *val, void *udata)
{
    (void)udata;
    if (child_info->type == CSONPATH_STR) {
        json_object_object_add(ctx, child_info->key, json_object_new_int(42));
    } else {
        json_object_array_put_idx(ctx, child_info->idx, json_object_new_int(42));
    }
    (void)val;
}

static void test_update_or_create_callback_missing_path(void)
{
    struct csonpath *p = csonpath_new("$.x.y.z");
    assert(p);
    struct json_object *jobj = json_tokener_parse("{}");
    int ret = csonpath_update_or_create_callback(p, jobj,
                                                  json_c_callback_set_42, NULL);
    assert(ret == 1);
    struct json_object *x = json_object_object_get(jobj, "x");
    assert(x);
    struct json_object *y = json_object_object_get(x, "y");
    assert(y);
    struct json_object *z = json_object_object_get(y, "z");
    assert(z && json_object_get_int(z) == 42);
    csonpath_destroy(p);
    json_object_put(jobj);
}

/* -------------------------------------------------------------------- */
/* 14. Subpath string key not found                                     */
/* -------------------------------------------------------------------- */

static void test_subpath_string_key_not_found(void)
{
    struct csonpath *p = csonpath_new("$.a[$.missing]");
    assert(p);
    struct json_object *jobj = json_tokener_parse(
        "{\"a\":{\"k\":1}, \"missing\": null}");
    CSONPATH_JSON ret = csonpath_find_first(p, jobj);
    assert(ret == NULL);
    csonpath_destroy(p);
    json_object_put(jobj);
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

    printf("\n-- Recursive descent on scalars / arrays --\n"); fflush(stdout);
    RUN(test_recursive_descent_on_scalar);
    RUN(test_recursive_descent_on_empty_obj);
    RUN(test_recursive_descent_into_array);

    printf("\n-- Union edge cases --\n"); fflush(stdout);
    RUN(test_empty_union);
    RUN(test_union_mixed_types);
    RUN(test_union_find_all);

    printf("\n-- Getter on non-object --\n"); fflush(stdout);
    RUN(test_get_obj_on_array);
    RUN(test_get_obj_on_scalar);

    printf("\n-- Filter and range edge cases --\n"); fflush(stdout);
    RUN(test_filter_on_scalar);
    RUN(test_range_on_non_array);
    RUN(test_range_basic);
    RUN(test_range_find_all);
    RUN(test_range_update_or_create);
    RUN(test_range_remove);

    printf("\n-- Array remove / update --\n"); fflush(stdout);
    RUN(test_remove_array_element);
    RUN(test_remove_array_all);
    RUN(test_remove_array_out_of_bounds);
    RUN(test_update_array_element);
    RUN(test_update_array_out_of_bounds_gaps);
    RUN(test_update_array_big_index);
    RUN(test_update_array_element_by_subpath_index);
    RUN(test_remove_array_element_by_subpath_index);

    printf("\n-- Recursive descent remove --\n"); fflush(stdout);
    RUN(test_remove_recursive_descent);

    printf("\n-- Mutation during callback iteration --\n"); fflush(stdout);
    RUN(test_callback_mutation_next_key);

    printf("\n-- Deep nesting / stack stress --\n"); fflush(stdout);
    RUN(test_deep_recursive_descent);
    RUN(test_wide_array_get_all);

    printf("\n-- set_path --\n"); fflush(stdout);
    RUN(test_set_path_invalid_then_valid);

    printf("\n-- broken path object --\n"); fflush(stdout);
    RUN(test_find_first_on_broken_path);

    printf("\n-- Callback and update_or_create_callback --\n"); fflush(stdout);
    RUN(test_callback_basic);
    RUN(test_update_or_create_callback_missing_path);

    printf("\n-- Subpath string key not found --\n"); fflush(stdout);
    RUN(test_subpath_string_key_not_found);

    printf("\n=== All crash-vector tests passed ===\n"); fflush(stdout);
    return 0;
}
