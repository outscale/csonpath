#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "csonpath_json-c.h"

/* Reproducer tests for bugs found during audit.
 * These tests describe desired behaviour.
 * Some currently crash or return incorrect results.
 */

/* Bug 1: remove("$") segfaults because child_info is uninitialized
 * when the path is just ROOT -> END.
 */
void test_remove_root_does_not_crash(void)
{
    struct csonpath *p = csonpath_new("$");
    struct json_object *jobj = json_tokener_parse("{\"a\": 1}");
    int removed;

    assert(p);
    assert(jobj);
    /* This call must not crash and should remove nothing (root is not deletable). */
    removed = csonpath_remove(p, jobj);
    printf("remove($) returned %d (expect 0)\n", removed);
    assert(removed == 0);
    json_object_put(jobj);
    csonpath_destroy(p);
}

/* Bug 2: remove on deep path should only delete target key. */
void test_remove_deep_path_on_nested_obj(void)
{
    struct csonpath *p = csonpath_new("$.a.b");
    struct json_object *jobj = json_tokener_parse("{\"a\": {\"b\": 1, \"c\": 2}}");
    int removed;

    assert(p);
    assert(jobj);
    removed = csonpath_remove(p, jobj);
    printf("remove($.a.b) returned %d\n", removed);
    assert(removed == 1);
    assert(json_object_object_get(json_object_object_get(jobj, "a"), "b") == NULL);
    assert(json_object_object_get(json_object_object_get(jobj, "a"), "c") != NULL);
    json_object_put(jobj);
    csonpath_destroy(p);
}

int main(void)
{
    printf("=== Audit bug reproducers ===\n");

    test_remove_root_does_not_crash();
    test_remove_deep_path_on_nested_obj();

    printf("=== All audit tests passed ===\n");
    return 0;
}
