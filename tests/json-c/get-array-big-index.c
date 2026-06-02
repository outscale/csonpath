#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "csonpath_json-c.h"

/*
 * Regression test for a bug in GET_ARRAY_BIG (index >= 100) handling.
 *
 * The old code used "walker += 5" for GET_ARRAY_BIG. Because the loop
 * also does "++walker" at "next_inst:", the walker over-advanced by one
 * byte, skipping the next instruction entirely.
 *
 * This test uses an existing array with 101 elements so that
 * update_or_create does not need to create intermediate containers
 * (avoiding reference-count issues in the json-c binding).
 */
int main(void)
{
    /* Build { "a": [ { "b": "wrong" }, ..., { "b": "hello" } ] } */
    struct json_object *root = json_object_new_object();
    struct json_object *arr = json_object_new_array();
    for (int i = 0; i < 101; ++i) {
        struct json_object *obj = json_object_new_object();
        if (i == 100)
            json_object_object_add(obj, "b", json_object_new_string("hello"));
        else
            json_object_object_add(obj, "b", json_object_new_string("wrong"));
        json_object_array_add(arr, obj);
    }
    json_object_object_add(root, "a", arr);

    struct csonpath *cp = csonpath_new("$.a[100].b");
    assert(cp);

    struct json_object *ret = csonpath_find_first(cp, root);
    assert(ret != NULL);
    assert(!strcmp(json_object_get_string(ret), "hello"));

    csonpath_destroy(cp);
    json_object_put(root);

    printf("OK\n");
    return 0;
}
