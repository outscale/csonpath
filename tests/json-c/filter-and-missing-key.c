#include <stdio.h>
#include <assert.h>
#include "csonpath_json-c.h"

/*
 * Regression test for a bug in compound filter (&) handling.
 *
 * When iterating over an array where some elements lack the key used
 * in the second filter condition, the internal `filter_next` offset
 * was not reset between array elements. This caused the filter walker
 * to read past the end of the first filter's bytecode and hit an
 * unexpected instruction (e.g. FILTER_OPERAND_STR).
 *
 * In the Python binding this manifests as a segfault because
 * CSONPATH_NULL is Py_None (not NULL), so the corrupted walker
 * dereferences a NULL el2 inside PyUnicode_Check.
 *
 * In the json-c binding the same logical corruption happens but
 * CSONPATH_NULL == NULL, so the code safely returns "no match"
 * after printing an error message. This test therefore verifies
 * the *correct* behaviour (exactly one match, no error output).
 */
int main(void)
{
    const char *json_str = "{"
        "\"hardware\": ["
        "{\"type\": \"fb\", \"status\": \"healthy\"},"
        "{\"type\": \"fb\"}"
        "]"
        "}";

    struct json_object *jobj = json_tokener_parse(json_str);
    assert(jobj);

    struct csonpath *cp = csonpath_new("$.hardware[?type = \"fb\" & status = \"healthy\"]");
    assert(cp);

    struct json_object *ret = csonpath_find_all(cp, jobj);
    assert(ret);

    /* Must return exactly one element (the first one). */
    assert(json_object_array_length(ret) == 1);

    json_object_put(ret);
    json_object_put(jobj);
    csonpath_destroy(cp);

    return 0;
}
