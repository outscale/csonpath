/*
 * FILTER_KEY_EXIST regression coverage.
 *
 * `?key` is compiled as `?key != null`: FILTER_KEY_EXIST is only ever the
 * 1-byte null operand, never a filter opcode.  The cases below exercise the
 * two bytecode walkers that used to derail when they had to step over a
 * filter operand: CSONPATH_DO_GET_NOTFOUND (missing parent) and the union
 * skip paths (csonpath_skipp_union_jmp / GET_UNION).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <json-c/json.h>
#include "csonpath_json-c.h"

static struct json_object *root;

static struct json_object *find_all(const char *path)
{
    struct csonpath *p = csonpath_new(path);
    struct json_object *ret;

    if (!p) {
        fprintf(stderr, "filter-exist: cannot compile %s\n", path);
        abort();
    }
    ret = csonpath_find_all(p, root);
    csonpath_destroy(p);
    return ret;
}

static int matches(const char *path)
{
    struct json_object *ret = find_all(path);
    int n;

    if (!ret)
        return -1;
    assert(json_object_is_type(ret, json_type_array));
    n = (int)json_object_array_length(ret);
    json_object_put(ret);
    return n;
}

static void expect(const char *path, int n)
{
    int got = matches(path);

    if (got != n) {
        fprintf(stderr, "filter-exist: %s: expected %d match(es), got %d\n",
                path, n, got);
        abort();
    }
}

static void load(const char *json)
{
    root = json_tokener_parse(json);
    assert(root);
}

static void release(void)
{
    json_object_put(root);
    root = NULL;
}

int main(void)
{
    /* --- 1. existence vs null -------------------------------------- */
    load("["
         "{},"
         "{\"a\":null},"
         "{\"a\":0},"
         "{\"a\":false},"
         "{\"a\":\"\"},"
         "{\"a\":[]},"
         "{\"a\":{}},"
         "{\"b\":1},"
         "{\"a\":{\"b\":1}},"
         "{\"a\":{\"b\":null}},"
         "{\"a\":[1,2]},"
         "{\"a\":1,\"b\":2,\"c\":3}"
         "]");

    /* ?key is ?key != null: JSON null is not an existing value. */
    expect("$[?a]", 9);
    expect("$[?(@.a)]", 9);
    expect("$[?(@[\"a\"])]", 9);
    expect("$[?a!=null]", 9);
    expect("$[?a==null]", 3);          /* absent, null, b-only */
    expect("$[?a.b]", 1);              /* {"a":{"b":1}} only */
    expect("$[?(@.a.b)]", 1);
    expect("$[?z]", -1);

    /* unions must evaluate every branch */
    expect("$[?a,?b]", 11);
    expect("$[?a,?b,?c]", 12);         /* duplicate union hits */

    release();

    /* --- 2. true / false operands ---------------------------------- */
    load("["
         "{\"a\":true},"
         "{\"a\":false},"
         "{\"a\":1},"
         "{\"a\":0},"
         "{\"a\":\"true\"},"
         "{\"a\":null},"
         "{}"
         ",{\"b\":true}"
         ",{\"true\":1}"
         ",{\"false\":0}"
         ",{\"a\":true,\"b\":1}"
         "]");

    expect("$[?a==true]", 2);
    expect("$[?a==false]", 1);
    expect("$[?a!=true]", 9);
    expect("$[?a!=false]", 10);
    expect("$[?true]", 1);             /* existence of key "true" */
    expect("$[?(@.true)]", 1);
    expect("$[?false]", 1);            /* existence of key "false" */
    expect("$[?a==true,?b]", 4);
    expect("$[?a==true&&?b]", 1);
    release();

    /* --- 3. union containing a null operand ------------------------ */
    load("{\"arr\":["
         "{\"a\":1,\"b\":2},"
         "{\"a\":null,\"b\":2},"
         "{\"a\":1},"
         "{\"b\":2},"
         "{}"
         "]}");

    /* Before the fix the second branch was silently skipped (or hung). */
    expect("$.arr[?a==null,?b]", 6);
    expect("$.arr[?b,?a==null]", 6);
    expect("$.arr[?b,?a!=null]", 5);
    expect("$.arr[?a,?b]", 5);
    expect("$.arr[?a,?b,?c]", 5);
    expect("$.arr[?a==null&&?b]", 2);
    release();

    /* --- 4. skipping over filters (missing parent) ----------------- */
    /* These paths used to hang / SIGBUS / SIGSEGV in GET_NOTFOUND. */
    load("{\"x\":1,\"arr\":[{\"a\":1},{\"a\":null}]}");

    expect("$.missing[?a]", -1);
    expect("$.missing[?(@.a.b)]", -1);
    expect("$.missing[?a.b]", -1);
    expect("$.missing[?a==null]", -1);
    expect("$.missing[?a!=null]", -1);
    expect("$.missing[?a==true]", -1);
    expect("$.missing[?a==false]", -1);
    expect("$.missing[?a,?b]", -1);
    expect("$.missing[?a,?b,?c]", -1);
    expect("$.missing[?b,?a==null]", -1);
    expect("$.missing[?(@.a==null)]", -1);
    expect("$.missing[?(@.a.b==null)]", -1);
    expect("$.missing[?a==true&&?b]", -1);
    expect("$.missing.x[?a]", -1);
    expect("$.missing[0][?a]", -1);
    expect("$.missing[?a].x", -1);
    expect("$.missing[?a==null].x", -1);
    release();

    return 0;
}
